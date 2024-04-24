#include "parser.hpp"

bool Parser::does_frame_have_features(
    Frame candidate_frame,
    bool is_left,
    Frame &consumer_frame)
{
    vector<FeatureTag> feature_tags;
    int element_index;
    if (is_left)
    {
        element_index = 0;
    }
    else
    {
        element_index = 1;
    }
    PatternElement pattern_element = consumer_frame.pattern_elements[element_index];
    feature_tags = pattern_element.feature_tags;

    // first test the regular tags
    for (FeatureTag test_feature_tag : feature_tags)
    {
        set<string> tag_set = candidate_frame.feature_set;
        if (test_feature_tag.tag_type == FeatureTagType::Necessary)
        {
            // look for feature in frame
            if (tag_set.count(test_feature_tag.feature_name) == 0)
                return false;
        }
        else
        {
            // look for absence of feature in frame
            if (tag_set.count(test_feature_tag.feature_name) != 0)
                return false;
        }
    }

    vector<string> feature_group_tags = pattern_element.feature_group_tags;
    // then test for feature groups
    for (string feature_group_tag : feature_group_tags)
    {
        vector<string> features_to_check_for = grammar.feature_group_to_features.at(feature_group_tag);

        bool any_feature_matches_group = false;
        for (string feature_to_check_for : features_to_check_for)
        {
            if (candidate_frame.feature_set.count(feature_to_check_for) != 0)
            {
                any_feature_matches_group = true;
                // modify the consumer frame's tag. also modify the subsequent appearances of this feature group tag
                consumer_frame.feature_set.emplace(feature_to_check_for);
                if (element_index == 0)
                {
                    // before appending a feature, check if the subsequent pattern element has a feature_group_tag
                    bool does_have_feature_group = false;
                    for (string potential_matching_feature_group : consumer_frame.pattern_elements[1].feature_group_tags)
                    {
                        if (equals(potential_matching_feature_group, feature_group_tag))
                        {
                            does_have_feature_group = true; // todo change the feature_group_tags to a set instead of a vector for perf improvement
                            continue;
                        }
                    }
                    if (does_have_feature_group)
                        consumer_frame.pattern_elements[1].feature_tags.push_back(
                            FeatureTag(
                                feature_to_check_for,
                                FeatureTagType::Necessary));
                }
                any_feature_matches_group = true;
            }
        }
        if (!any_feature_matches_group)
            return false; // consider changing this if the behavior is unexpected
    }
    return true;
}

bool Parser::get_matched_frames(
    Frame left_consumer_frame,
    Frame right_consumer_frame,
    vector<Frame> &matched_frames)
{
    // update to vector<Frame>
    string left_string;
    if (left_consumer_frame.is_word_frame())
    {
        // is a word
        left_string = left_consumer_frame.get_part_of_speech();
    }
    else
    {
        left_string = left_consumer_frame.frame_name;
    }

    string right_string;
    if (right_consumer_frame.is_word_frame())
    {
        // is a word
        right_string = right_consumer_frame.get_part_of_speech();
    }
    else
    {
        right_string = right_consumer_frame.frame_name;
    }

    string match_string = left_string + " " + right_string;
    // printf("finding matching frames - '%s'\n", match_string.c_str());

    // printf("match string: %s\n", match_string.c_str());
    if (!(grammar.cnf_map.find(match_string) == grammar.cnf_map.end()))
    {
        vector<Frame> frames_to_doublecheck = grammar.cnf_map.at(match_string);

        // vector<Frame> accepted_frames;
        for (int frame_index = 0; frame_index < frames_to_doublecheck.size(); frame_index++)
        {
            Frame candidate_frame = frames_to_doublecheck[frame_index];

            // perform checks on PoS type and features

            // feature tag check
            PatternElement left_pattern_element = candidate_frame.pattern_elements[0];
            PatternElement right_pattern_element = candidate_frame.pattern_elements[1];

            vector<FeatureTag> left_feature_tags = left_pattern_element.feature_tags;
            vector<FeatureTag> right_feature_tags = right_pattern_element.feature_tags;

            if (does_frame_have_features(left_consumer_frame, true, candidate_frame) && does_frame_have_features(right_consumer_frame, false, candidate_frame))
                matched_frames.push_back(
                    candidate_frame.with_links(
                        left_frame_coordinates,
                        right_frame_coordinates));
        }

        // for (int i = 0; i < accepted_frames.size(); i++){
        // 	matched_frames.push_back(accepted_frames.at(i));
        // }
        return true;
        // printf("\tthe frame's name: %s\n", found_frame.frame_name.c_str());
    }
    return false;

}

vector<Frame> Parser::find_matching_frames(vector<Frame> left_frames, vector<Frame> right_frames)
{
    vector<Frame> matching_frames;
    for (int left_index = 0; left_index < left_frames.size(); left_index++)
    {
        Frame left_frame = left_frames.at(left_index);
        for (int right_index = 0; right_index < right_frames.size(); right_index++)
        {
            Frame right_frame = right_frames.at(right_index);

            left_frame_coordinates.num = left_index;
            right_frame_coordinates.num = right_index;

            vector<Frame> matched_frames;
            if (get_matched_frames(left_frame, right_frame, matched_frames))
            {
                for (int match_index = 0; match_index < matched_frames.size(); match_index++)
                {
                    matching_frames.push_back(matched_frames.at(match_index));
                }
                // printf("found %ld matchs\n", matched_frames.size());
            }
        }
    }

    return matching_frames;
}

Parser::Parser(Grammar grammar) : grammar(grammar)
{
    is_highlighted = false;
}

Parser::Parser() {}

void Parser::update_parse_grid(string new_utterance)
{
    current_utterance = new_utterance;

    // tokenize the utterance
    vector<string> split_tokens;
    boost::split(split_tokens, current_utterance, boost::is_any_of(" "), boost::token_compress_on);
    int token_count = split_tokens.size();

    // initialize the parse_grid
    vector<vector<vector<Frame>>> new_grid;
    for (int row = 0; row < token_count; row++)
    {
        vector<vector<Frame>> new_row;
        for (int col = 0; col < token_count - row; col++)
        {
            vector<Frame> new_cell;
            new_row.push_back(new_cell);
        }
        new_grid.push_back(new_row);
    }

    parse_grid = new_grid;

    // populate the bottom row with the word frames of the utterance
    for (int token_index = 0; token_index < token_count; token_index++)
    {
        string token = split_tokens[token_index];
        bool does_match = !(grammar.word_map.find(token) == grammar.word_map.end());

        if (does_match)
        {
            vector<Frame> word_frames_identified = grammar.word_map.at(token);
            for (Frame word_frame : word_frames_identified)
            {
                parse_grid[0][token_index].push_back(word_frame);
            }
        }
    }

    if (token_count < 2)
        return;

    // perform cyk algo
    printf("parsing grammar\n");
    for (int row = 1; row < token_count; row++)
    {
        for (int col = 0; col < token_count - row; col++)
        {
            vector<Frame> potential_frames;

            // use the cnf

            // X         4
            // 0 3       3
            // 1   2     2
            // 2_____1   1
            // 3       0 0

            // X__
            // L  R

            for (int pair_index = 0; pair_index < row; pair_index++)
            {
                int left_row = row - (pair_index + 1);
                vector<Frame> left_frames = parse_grid[left_row][col];

                int step_diagonal = row - pair_index;
                int right_row = row - step_diagonal;
                int right_col = col + step_diagonal;
                vector<Frame> right_frames = parse_grid[right_row][right_col];

                left_frame_coordinates = FrameCoordinates(left_row, col, -1);
                right_frame_coordinates = FrameCoordinates(right_row, right_col, -1);

                vector<Frame> matching_frames = find_matching_frames(left_frames, right_frames);
                for (Frame matching_frame : matching_frames)
                {
                    // printf("adding matched frame named %s. to row %d, col %d\n", matched_frame.frame_name.c_str(), row, col);
                    parse_grid[row][col].push_back(matching_frame);
                }
            }
        }
    }
}

bool Parser::try_get_top_interpretation(Frame& interp_frame)
{
    auto parse_grid_top_cell = parse_grid[parse_grid.size()-1][0];
    if (parse_grid_top_cell.size() == 0)
    {
        return false;
    }
    interp_frame = parse_grid_top_cell[0];

    return true;
}

bool Parser::try_get_frame_at(FrameCoordinates coords, Frame& result_frame)
{
    // maybe the problem is the passing by reference here?
    if (!coords.is_empty())
    {
        result_frame = parse_grid[coords.row][coords.col][coords.num];
        return true;
    }
    return false;
}
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
    
    // this is where the overlapping logic for featuregroups has to come in.

    // refactor this code. rename consumer to something better like, source. source and product frame. bottom-up
    // make the feature group tags check with a set of some sort.
    // TODO2000
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

void Parser::load_frame(FrameCoordinates coords, Frame new_frame)
{
    vector<Frame> frames_to_add = vector<Frame>{new_frame};
    // if (DEBUGGING)
    // 	printf("checking if %s is in monoframes_by_right\n", word_frame.frame_name.c_str());

    // for (string feature : new_frame.feature_set)
    // {

        
    // frames_to_add.push_back(new_frame);
        
    // printf("new frame: %s\n", new_frame.stringify_pre_binarization().c_str());
    if (grammar.monoframes_by_pattern_element.count(new_frame.frame_name) != 0)
    {
        printf("A MATCH HAS OCCURED\n");
        // add all product monoframes to this word mapping
        // for (auto pattern_and_monoframe : grammar.monoframes_by_right.at(new_frame.frame_name))
        // {
        //     Frame result_frame = pattern_and_monoframe.second;
        //     PatternElement pattern_element = pattern_and_monoframe.first;

        //     result_frame.type = FrameType::Derived;

        //     // check if features match here.
        //     // for now just check features, will support feature groups for verbs
        //     // printf("result_frame frame: %s", result_frame.stringify_pre_binarization().c_str());
        //     result_frame.accumulated_expression = PredicateRuleApplier::apply_formation_rules_on_expression(
        //     	RuleApplierContext(predicate_handler, variable_namer),
        //     	result_frame.predicate_formation_rules,
        //     	result_frame.accumulated_expression,
        //     	vector<Frame>{result_frame}
        //     );

        //     // if (DEBUGGING)
        //         // printf("adding a frame %s\n", result_frame.pattern_elements[0].stringify().c_str());
        //     frames_to_add.push_back(result_frame);
        //     // add_to_word_map(result_frame, match_string);
        // }
    }
    // }

    for (auto frame_to_add : frames_to_add)
    {
        printf("PUTTING IN THE BACK SOMETHIGN TERRIBLE, namely frame: %s\n", frame_to_add.stringify_pre_binarization().c_str());
        parse_grid[coords.row][coords.col].push_back(frame_to_add);
    }


}

bool Parser::try_get_matched_frames(
    Frame left_candidate_frame,
    Frame right_candidate_frame,
    vector<Frame> &matched_frames)
{
    // update to vector<Frame>
    string left_string;
    if (left_candidate_frame.is_word_frame())
    {
        // is a word
        left_string = left_candidate_frame.get_part_of_speech();
    }
    else
    {
        left_string = left_candidate_frame.frame_name;
    }

    string right_string;
    if (right_candidate_frame.is_word_frame())
    {
        // is a word
        right_string = right_candidate_frame.get_part_of_speech();
    }
    else
    {
        right_string = right_candidate_frame.frame_name;
    }

    string match_string = left_string + " " + right_string;
    // printf("finding matching frames - '%s'\n", match_string.c_str());

    // printf("match string: %s\n", match_string.c_str());
    if (grammar.cnf_map.count(match_string) != 0)
    {
        vector<Frame> frames_to_doublecheck = grammar.cnf_map.at(match_string);

        // vector<Frame> accepted_frames;
        for (int frame_index = 0; frame_index < frames_to_doublecheck.size(); frame_index++)
        {
            Frame candidate_frame = frames_to_doublecheck[frame_index];

            if (candidate_frame.type == FrameType::Derived)
            {
                printf("the derived frame is: %s\n", candidate_frame.stringify_pre_binarization().c_str());
            }

            // perform checks on PoS type and features

            // feature tag check
            PatternElement left_pattern_element = candidate_frame.pattern_elements[0];
            PatternElement right_pattern_element = candidate_frame.pattern_elements[1];

            vector<FeatureTag> left_feature_tags = left_pattern_element.feature_tags;
            vector<FeatureTag> right_feature_tags = right_pattern_element.feature_tags;

            if (does_frame_have_features(left_candidate_frame, true, candidate_frame) && does_frame_have_features(right_candidate_frame, false, candidate_frame))
            {
                Expression new_expression = apply_formation_rules_on_frames(
                                            candidate_frame.predicate_formation_rules,
                                            left_candidate_frame,
                                            right_candidate_frame) ;
                matched_frames.push_back(
                    candidate_frame.with_links(
                        left_frame_coordinates,
                        right_frame_coordinates)
                                   .with_expression(new_expression));
            }
        }

        return true;
    }
    return false;

}

Expression Parser::apply_formation_rules_on_frames(PredicateFormationRules formation_rule, Frame left_frame, Frame right_frame)
{
    // first, combine the two expressions
    Expression combined_expression = Expression::combine_expressions(left_frame.accumulated_expression, right_frame.accumulated_expression);
    // auto expression_predicates = combined_expressions.predicates;

    auto context = RuleApplierContext(predicate_handler, variable_namer);

    return PredicateRuleApplier::apply_formation_rules_on_expression(context, formation_rule, combined_expression, vector<Frame>{left_frame, right_frame});
}

vector<Frame> Parser::find_matching_frames(vector<Frame> left_frames, vector<Frame> right_frames)
{
    vector<Frame> matching_frames;
    for (int left_index = 0; left_index < left_frames.size(); left_index++)
    {
        Frame left_frame = left_frames.at(left_index);

        // if (right_frames.size() == 0)
        // {
        //     // this is a derived monoframe
        //     vector<Frame> matched_frames;
            
        //     if (try_get_matched_frames(left_frame, right_frame, matched_frames))
        //     {
        //         for (int match_index = 0; match_index < matched_frames.size(); match_index++)
        //         {
        //             matching_frames.push_back(matched_frames.at(match_index));
        //         }
        //     }
        // }
        // else 
        // {
            for (int right_index = 0; right_index < right_frames.size(); right_index++)
            {
                Frame right_frame = right_frames.at(right_index);

                left_frame_coordinates.num = left_index;
                right_frame_coordinates.num = right_index;

                vector<Frame> matched_frames;
                if (try_get_matched_frames(left_frame, right_frame, matched_frames))
                {
                    for (int match_index = 0; match_index < matched_frames.size(); match_index++)
                    {
                        matching_frames.push_back(matched_frames.at(match_index));
                    }
                }
            }
        // }
    }

    return matching_frames;
}

// void Parser::get_matching_words(vector<Frame> word_frames, Frame consumer_frame, vector<Frame> &matching_frames)
// {
//     for (auto word_frame : word_frames)
//     {
//         find_matching_frames()
//     }
//     return vector<Frame>();
// }

Parser::Parser(Grammar grammar, VariableNamer* variable_namer) : grammar(grammar), variable_namer(variable_namer)
{
}

Parser::Parser() {}

void Parser::update_parse_grid(string new_utterance)
{
    current_utterance = new_utterance;

    if (DEBUGGING)
    {
        printf("parsing utterance %s\n", new_utterance.c_str());
    }

    // tokenize the utterance
    vector<string> split_tokens;
    split_tokens = split_spaces(current_utterance);
    int token_count = split_tokens.size();

    // initialize the parse_grid
    vector<vector<vector<Frame>>> new_grid;
    for (int row = 0; row < token_count; row++)
    {
        vector<vector<Frame>> new_row;
        for (int col = 0; col < token_count - row; col++)
        {
            vector<Frame> new_cell = vector<Frame>(30);
            new_row.push_back(new_cell);
        }
        new_grid.push_back(new_row);
    }

    if (DEBUGGING)
        printf("parse grid initialized\n");

    parse_grid = new_grid;

    // populate the bottom row with the word frames of the utterance
    for (int token_index = 0; token_index < token_count; token_index++)
    {
        string token = split_tokens[token_index];
        bool does_match = grammar.word_map.count(token) != 0;

        if (does_match)
        {
            vector<Frame> word_frames_identified = grammar.word_map.at(token);
            // printf("does match: %s\n", word_frames_identified[0].stringify_as_param().c_str());
            for (Frame word_frame : word_frames_identified)
            {
                printf("accessing %s\n", word_frame.stringify_as_param().c_str());

                // if(word_frame.type == FrameType::Derived)
                //     printf("predicate: %s\n", predicate_handler->stringify_expression(word_frame.accumulated_expression).c_str());

                printf("wordframe type %d\n", word_frame.type);
                printf("flag\n");
                if (word_frame.type != FrameType::Word && word_frame.type != FrameType::Derived)
                    throw runtime_error("syntax frame on word frame row not allowed");
                
                // if (word_frame.type == FrameType::Derived)
                // {
                //     auto derived_frame = word_frame;
                //     // do the matching
                //     // PatternElement pat_element = word_frame.pattern_elements[0];
                //     // string match_type = word_frame.pattern_elements[0].match_string;

                //     // for (Frame other_word_frame : word_frames_identified)
                //     // {
                //         // printf("other word frame: %s, matchType; %s\n", other_word_frame.frame_name.c_str(), match_type.c_str());
                //         // if (other_word_frame.feature_set.count(match_type) != 0)
                //         // {
                //     printf("matched on derived type %s\n", derived_frame.stringify_pre_binarization().c_str());

                //     auto word_frames_of_matching_type = find_matching_frames(word_frames_identified, vector<Frame>());

                //     for (auto matching_word_frame : word_frames_of_matching_type)
                //     {

                //         // if (grammar.monoframe_to_base_frame_map.count(derived_frame) != 0)
                //         // {
                //         // word_frame = grammar.monoframe_to_base_frame_map[derived_frame];
                //         printf("THE WORD FRAME IS: %s\n", word_frame.stringify_pre_binarization().c_str());

                //         // printf("the ")
                        
                //         derived_frame.accumulated_expression = PredicateRuleApplier::apply_formation_rules_on_expression(
                //             RuleApplierContext(predicate_handler, variable_namer),
                //             derived_frame.predicate_formation_rules,
                //             derived_frame.accumulated_expression,
                //             vector<Frame> {matching_word_frame}
                //         );

                //         printf("EXPRESSION: %s\n===================================================\n", predicate_handler->stringify_expression(derived_frame.accumulated_expression).c_str());

                //         derived_frame.type = FrameType::Derived;

                //         // find the index of the frame mentioned
                //         // derived_frame.left_match = FrameCoordinates(0, token_index, )

                //         // um, hoOW do i get access to the base word frame? 
                //         // add a new property to 
                //         printf("adding derived word frame\n");
                //         // parse_grid[0][token_index].push_back(derived_frame);

                //             // }
                //         // }

                //         // }
                //         load_frame(FrameCoordinates(0, token_index, -1), derived_frame);
                //         // parse_grid[0][token_index].push_back(derived_frame);
                //     }
                // }
                // else {
                load_frame(FrameCoordinates(0, token_index, -1), word_frame);
                    // parse_grid[0][token_index].push_back(word_frame);
                // }

            }
        }
    }

    if (DEBUGGING)
        printf("bottom row of parse grid populated");

    if (token_count < 2)
        return;

    // perform cyk algo
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
                    load_frame(FrameCoordinates(row, col, -1), matching_frame);
                    // parse_grid[row][col].push_back(matching_frame);
                }
            }
        }
    }
}

bool Parser::try_get_top_frame(Frame& interp_frame)
{
    auto parse_grid_top_cell = parse_grid[parse_grid.size()-1][0];
    if (parse_grid_top_cell.size() == 0)
    {
        return false;
    }
    interp_frame = parse_grid_top_cell[0];

    return true;
}

vector<Frame> Parser::get_interpret_frames()
{
    vector<Frame> fetched_frames = vector<Frame>();
    for (auto frame : parse_grid[parse_grid.size()-1][0])
    {
        fetched_frames.push_back(frame);
    }
    return fetched_frames;
}

vector<Frame> Parser::get_interpret_frames(int x_coord, int y_coord)
{
    vector<Frame> fetched_frames = vector<Frame>();

    vector<vector<Frame>> target_row = parse_grid[y_coord];
    if (x_coord >= target_row.size())
        throw runtime_error("bad x coordinate when accessing interpret frames");
    


    for (auto frame : target_row[x_coord])
    {
        fetched_frames.push_back(frame);
    }
    return fetched_frames;
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

string Parser::stringify()
{
    string buildee = "";
    for (int row = 1; row < parse_grid.size(); row++)
    {\
        buildee += "|";
        for (int col = 0; col < parse_grid.size() - row; col++)
        {
            vector<Frame> matches = parse_grid[row][col];
            bool is_more_than_one = matches.size() > 1;
            if (matches.size() != 0)
            {
                buildee += " ";
                buildee += matches[0].frame_nickname;
                buildee += is_more_than_one ? "*" : " ";
            }
            else
            {
                buildee += "     ";
            }

            buildee += "|\n";
        }
    }
    return buildee;
}

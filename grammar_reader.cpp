#include "grammar_reader.hpp"

void GrammarReader::add_term_forms(
    vector<PatternNecessity> &term_form_types,
    vector<string> &term_form_names)
{
    term_form_types.clear();
    term_form_names.clear();
    // reading a type, to be followed by indent
    type_heirarchy.push_back(first_token.substr(0, first_token.size() - 1));

    // check for formlist formatted like this:
    // 		PoSType: #form1 form2 (form3)
    if (split_tokens.size() > 1 && split_tokens[1].at(0) == '#')
    {
        for (int i = 2; i < split_tokens.size(); i++)
        {
            string term_form_string = split_tokens[i];
            if (term_form_string.at(0) == '(' && term_form_string.back() == ')')
            {
                // is optional
                term_form_types.push_back(PatternNecessity::Optional);
                term_form_names.push_back(term_form_string.substr(1, term_form_string.size() - 2));
            }
            else
            {
                // is required
                term_form_types.push_back(PatternNecessity::Required);
                term_form_names.push_back(term_form_string);
            }
        }
    }
    else
    {
        term_form_names.clear();
    }
}

void GrammarReader::read_syntax_entry()
{
    // determine if you're reading a name / nickname line or a pattern line
    // the second token of a pattern name line is quoted
    string second_token = split_tokens[1];
    bool is_pattern_name_line = (second_token.at(0) == '"' && second_token.back() == '"');

    if (is_pattern_name_line)
    {
        pattern_name = first_token;
        pattern_nickname = trim_front_and_back(second_token);
    }
    else
    {
        if (first_token[0] == '#')
        {
            // it's a comment, continue
            return;
        }
        // is a pattern frame

        vector<PatternElement> pattern_elements;
        set<string> features;
        set<string> feature_groups;
        for (int pattern_element_index = 0; pattern_element_index < split_tokens.size(); pattern_element_index++)
        {
            PatternNecessity necessity;
            string match_string = split_tokens[pattern_element_index];

            // check for - prefix indicating this being a feature
            if (match_string[0] == '-')
            {
                string feature_name = match_string.substr(1, match_string.size() - 1);
                // check if the feature has the name of a group
                features.emplace(feature_name);
                continue;
            }

            // check for parens
            bool is_optional = (match_string.at(0) == '(' && match_string.back() == ')');
            if (is_optional)
            {
                string no_parens = trim_front_and_back(match_string);

                match_string = no_parens;
                necessity = PatternNecessity::Optional;
            }
            else
            {
                necessity = PatternNecessity::Required;
            }

            // check for features
            // in this format:
            // <word>[<feature1>,<feature2>]

            vector<FeatureTag> feature_tags;
            vector<string> pattern_feature_groups;
            int feature_open_pos = match_string.find('[');
            if (feature_open_pos != -1)
            {
                // pattern element has feature tags
                string feature_string = match_string.substr(feature_open_pos, match_string.size());
                match_string = match_string.substr(0, feature_open_pos);

                feature_string = trim_front_and_back(feature_string);
                vector<string> feature_names;
                boost::split(feature_names, feature_string, boost::is_any_of(","), boost::token_compress_off);

                for (int feature_tag_index = 0; feature_tag_index < feature_names.size(); feature_tag_index++)
                {
                    string feature_name = feature_names[feature_tag_index];
                    trim(feature_name);
                    if (feature_name[0] == '!')
                    {
                        // negatives not allowed on feature groups - assuming this is a regular feature
                        feature_name = feature_name.substr(1, feature_name.size() - 1);
                        if (grammar->feature_group_set.count(feature_name) != 0)
                        {
                            throw invalid_argument("feature groups are not allowed to be inverted yet.\n");
                        }

                        feature_tags.push_back(FeatureTag(feature_name, FeatureTagType::Prohibited));
                    }
                    else
                    {
                        // check if feature name is that of a feature group
                        if (grammar->feature_group_set.count(feature_name) != 0)
                        {
                            pattern_feature_groups.push_back(feature_name);
                        }
                        else
                        {
                            feature_tags.push_back(FeatureTag(feature_name, FeatureTagType::Necessary));
                        }
                    }
                }
            }

            pattern_elements.push_back(PatternElement(
                match_string,
                "",
                necessity,
                feature_tags,
                pattern_feature_groups));
        }
        Frame new_pattern_frame = Frame(
            pattern_name,
            pattern_nickname,
            pattern_elements,
            features,
            feature_groups);

        grammar->syntax_frames.push_back(new_pattern_frame);
    }
}

void GrammarReader::read_feature_group_entry()
{
    // FORMAT:
    //	GROUPNAME= feature1 feature2 feature3
    string feature_group_name = split_tokens[0];
    // cut off the '='
    feature_group_name = feature_group_name.substr(0, feature_group_name.size() - 1);

    grammar->feature_group_set.emplace(feature_group_name);

    vector<string> feature_names;
    for (int i = 1; i < split_tokens.size(); i++)
    {
        string feature_name = split_tokens[i];
        feature_names.push_back(feature_name);

        grammar->feature_to_feature_group.emplace(feature_name, feature_group_name);
    }
    grammar->feature_group_to_features.emplace(feature_group_name, feature_names);
}

void GrammarReader::read_word_entry()
{
    // reading word

    // have a copy type_heirarchy for inserting
    vector<string> type_pruned = vector<string>(type_heirarchy.begin() + 1, type_heirarchy.end());

    // otherwise it is a list of word forms that corresponds to the current term
    // therefore, populate the hash set of words with the newly created word object
    if (split_tokens.size() >= 2 && split_tokens.at(1).at(0) == '-')
    {
        // reading a word with feature markers (should not have had word for indicated by #list)
        string word_string = split_tokens[0];
        vector<string> features;

        for (int i = 1; i < split_tokens.size(); i++)
        {
            string feature_name = split_tokens[i];
            features.push_back(feature_name.substr(1, feature_name.size()));
        }
        Frame new_word_frame = Frame(word_string, type_pruned, features);

        grammar->add_to_word_map(new_word_frame, word_string);
    }
    else
    {
        string base_word_form = split_tokens[0];
        for (int word_form_index = 0; word_form_index < split_tokens.size(); word_form_index++)
        {
            string word_string = split_tokens[word_form_index];
            // TODO - add mapping to base form of word for conceptual association

            Frame new_word_frame;

            if (term_form_names.size() < word_form_index + 1)
            {
                // no form list
                new_word_frame = Frame(word_string, type_pruned);
            }
            else
            {
                string word_form = term_form_names.at(word_form_index);
                new_word_frame = Frame(base_word_form, type_pruned, word_form);
            }
            grammar->add_to_word_map(new_word_frame, word_string);
        }
    }
}

GrammarReader::GrammarReader(Grammar *grammar_ptr)
    : grammar(grammar_ptr)
{
    state = GrammarReaderState::ReadingWords;
}

void GrammarReader::read_grammar(string fileName)
{

    fstream newfile;
    int tab_spaces = 4;

    printf("reading grammar\n");

    newfile.open(fileName, ios::in); // open a file to perform read operation using file object
    if (newfile.is_open())
    { // checking whether the file is open

        while (getline(newfile, current_line))
        { // read data from file object and put it into string.
            // printf("%s\n", current_line.c_str());
            if (current_line.size() == 0)
                continue;

            // measure indentation by counting initial spaces
            int initial_spaces = count_initial_spaces(current_line);
            int current_indentation = initial_spaces / tab_spaces;

            if (current_indentation < previous_indentation)
            {
                for (int i = 0; i < previous_indentation - current_indentation; i++)
                {
                    if (type_heirarchy.size() > 0)
                    {
                        type_heirarchy.pop_back();
                    }
                    term_forms.clear();
                    term_form_names.clear();
                }
            }
            previous_indentation = current_indentation;

            trim(current_line);

            if (equals(current_line, "FeatureGroups:"))
            {
                state = GrammarReaderState::ReadingFeatureGroups;
                continue;
            }
            if (equals(current_line, "Frames:"))
            {
                state = GrammarReaderState::ReadingSyntax;
            }

            boost::split(split_tokens, current_line, boost::is_any_of(" "), boost::token_compress_on);

            // if the first token ends in a ":" you're going up a level in the type_heirarchy
            first_token = split_tokens[0];
            if (first_token.at(first_token.size() - 1) == ':')
            {
                add_term_forms(term_forms, term_form_names);
            }
            else
            {
                switch (state)
                {
                case GrammarReaderState::ReadingWords:
                    read_word_entry();
                    break;
                case GrammarReaderState::ReadingSyntax:
                    read_syntax_entry();
                    break;
                case GrammarReaderState::ReadingFeatureGroups:
                    read_feature_group_entry();
                    break;
                }
            }
        }
        newfile.close(); // close the file object.
    }
}
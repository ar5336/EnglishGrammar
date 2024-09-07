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
    if (DEBUGGING)
        printf("reading syntax entry %s\n", current_line.c_str());
    
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

        PredicateFormationRules formation_rule = PredicateFormationRules();
        for (int pattern_element_index = 0; pattern_element_index < split_tokens.size(); pattern_element_index++)
        {
            PatternNecessity necessity;
            string match_string = split_tokens[pattern_element_index];

            // for now, the < predicateFromationRule * > is the final portion of the syntax entry
            if (match_string[0] == '<')
            {
                string combined_string = "";
                for (int i = pattern_element_index; i < split_tokens.size(); i++)
                {
                    string pred_form_rule_str = split_tokens[i];
                    if (i == pattern_element_index)
                    {
                        pred_form_rule_str = pred_form_rule_str.substr(1);

                    }

                    combined_string += pred_form_rule_str + " ";
                }
                auto rule_reader = PredicateRuleReader(predicate_handler);

                string trimmed_combined_string = combined_string.substr(0, combined_string.size()-2);

                rule_reader.try_read_predicate_rule(trimmed_combined_string, &formation_rule);
                break;
            }

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
                vector<string> feature_names = split_character(feature_string, ",");

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
                            throw runtime_error("feature groups are not allowed to be inverted yet.\n");
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
            current_line_index,
            pattern_nickname,
            pattern_elements,
            features,
            feature_groups,
            formation_rule);

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
                new_word_frame = Frame(word_string, current_line_index, type_pruned);
            }
            else
            {
                string word_form = term_form_names.at(word_form_index);
                new_word_frame = Frame(base_word_form, current_line_index, type_pruned, word_form);
            }
            grammar->add_to_word_map(new_word_frame, word_string);
        }
    }
}

void GrammarReader::read_predicate_template_entry()
{
    string predicate_name = split_tokens[0];

    vector<string> input_param_names(split_tokens.begin() + 1, split_tokens.end());
    auto are_params_schematic = vector<bool>();
    auto param_names = vector<string>();
    for (string param_name : input_param_names)
    {
        if (find_in_string(param_name, "->"))
        {
            param_names.push_back(param_name.substr(0, param_name.size()-2));
            are_params_schematic.push_back(true);
        }
        else
        {
            param_names.push_back(param_name);
            are_params_schematic.push_back(false);
        }
    }

    PredicateTemplate predicate_template = PredicateTemplate(predicate_name, param_names, are_params_schematic);

    predicate_template_handler->add_entry(predicate_template);
}

GrammarReader::GrammarReader(
    Grammar *grammar_ptr,
    PredicateHandler *predicate_handler_ptr,
    PredicateTemplateHandler *predicate_template_handler)
    : grammar(grammar_ptr), predicate_handler(predicate_handler_ptr), predicate_template_handler(predicate_template_handler)
{
    state = GrammarReaderState::ReadingWords;
}

void GrammarReader::read_grammar(string file_name)
{
    fstream new_file;
    int tab_spaces = 4;

    if (DEBUGGING)
        printf("reading grammar\n");

    new_file.open(file_name, ios::in); // open a file to perform read operation using file object
    if (new_file.is_open())
    { // checking whether the file is open

        while (getline(new_file, current_line))
        { // read data from file object and put it into string.
            // printf("%s\n", current_line.c_str());
            current_line_index++;

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
            if (equals(current_line, "Predicates:"))
            {
                state = GrammarReaderState::ReadingPredicateTemplates;
            }
            if (equals(current_line, "Frames:"))
            {
                predicate_handler->predicate_template_handler = predicate_template_handler;
                state = GrammarReaderState::ReadingSyntax;
            }

            split_tokens = split_spaces(current_line);

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
                case GrammarReaderState::ReadingPredicateTemplates:
                    read_predicate_template_entry();
                    break;
                }
            }
        }
        
        new_file.close(); // close the file object.
    }
}
#include "predicate_rule_reader.hpp"

PredicateRuleReader::PredicateRuleReader(PredicateHandler *predicate_handler) : predicate_handler(predicate_handler)
{
    // the predicate handler will definitely need to know the predicate classes, and as such should be assigned the taks of reading off predicate.txt
};

bool PredicateRuleReader::TryReadpredicateRule(string predicate_rule, PredicateFormationRules* formation_rules)
{
    auto predicate_creators = vector<PredicateCreator>();
    auto predicate_modifiers = vector<PredicateModifier>();
    vector<string> predicate_formation_strings = split_character(predicate_rule, "|");
    
    for (int i = 0; i < predicate_formation_strings.size(); i++)
    {
        string predicate_formation_string = predicate_formation_strings[i];

        trim(predicate_formation_string);
        vector<string> formation_tokens = split_spaces(predicate_formation_string);

        // determine if formation tokens belong to a creator or modifier
        if (predicate_formation_string.find('=') != -1)
        {
            // printf("ouh no?\n");
            predicate_modifiers.push_back(PredicateModifier(predicate_handler, predicate_formation_string));
            continue;
        }

        // ACTION actionVariable:! actor:NounPhrase->IS_INSTANCE_OF.object action:Verb
        printf("howdly\n");
        predicate_creators.push_back(PredicateCreator(predicate_handler, formation_tokens));
    }

    formation_rules->predicate_creators = predicate_creators;
    formation_rules->predicate_modifiers = predicate_modifiers;

    return true;
};

PredicateFormationRules::PredicateFormationRules()
{
    predicate_creators = vector<PredicateCreator>();
    predicate_modifiers = vector<PredicateModifier>();
}

PredicateFormationRules::PredicateFormationRules(vector<PredicateCreator> predicate_creators, vector<PredicateModifier> predicate_modifiers)
: predicate_creators(predicate_creators), predicate_modifiers(predicate_modifiers) {};

PredicateCreator::PredicateCreator(PredicateHandler *handler, vector<string> creation_tokens)
{
    string predicate_name = creation_tokens[0];

    printf("handler pointer: %p\n", handler);
    PredicateTemplate predicate_template = handler->GetPredicateTemplate(predicate_name);
    auto parameter_tokens = vector<string>(creation_tokens.begin() + 1, creation_tokens.end());    


    printf("creation token 0: %s\n", creation_tokens[0].c_str());
    for (int i = 0; i < parameter_tokens.size(); i++)
    {
        string parameter_token = parameter_tokens[i];

        auto parameter_split_colon = split_character(parameter_token, ":");
        if (parameter_split_colon.size() != 2)
            throw "parameter entry in predicate creation rule has problem with ':'";
        
        string argument = parameter_split_colon[1];

        string parameter_name = parameter_split_colon[0];

        if (parameter_name != predicate_template.parameter_names[i])
            throw "parameter name in predicate creation rule is named wrong";


        if (argument == "!")
        {
            parameter_creation_types.push_back(ParameterCreationType::WILDCARD);
            continue;
        }
        else if (find_in_string(argument, (string)"->"))
        {
            printf("found '->' in here: %s", argument.c_str());
            parameter_creation_types.push_back(ParameterCreationType::FRAME_PREDICATE_PROPERTY);

            pattern_predicate_accessors.push_back(PatternElementPredicateAccessor(handler, argument));
            continue;
        }
        // if (handler->PredIntFromString(argument) != -1)
        else
        {
            parameter_creation_types.push_back(ParameterCreationType::WORD_FRAME);
            // word_frame_accessors.push_back(WordFrameAccessor())
            continue;
        }

        // throw invalid_argument("predicate rule string does not match a defined type of predicateFormer's parameter type");
    }
}

PatternElementPredicateAccessor::PatternElementPredicateAccessor() {}

PatternElementPredicateAccessor::PatternElementPredicateAccessor(PredicateHandler* handler, string token)
{
    vector<string> parts = split_character(token, "->"); //TODO add to string_operators
    // verify that the split is correct 

    printf("token: %s\n", token.c_str());

    assert(parts.size() == 2);

    string syntax_frame_name = parts[0];
    // verify that predicate is actual pedicate
    // handler->GetPredicateTemplate(predicate_name);


    string leftover_token = parts[1];
    vector<string> predicate_and_parameter = split_character(leftover_token, ".");
    assert(predicate_and_parameter.size() == 2);

    predicate_name = predicate_and_parameter[0];
    parameter_name = predicate_and_parameter[1];
}

PredicateModifier::PredicateModifier(PredicateHandler* handler, string token)
{
    auto left_and_right = split_character(token, "=");

    left_equal = PatternElementPredicateAccessor(handler, left_and_right[0]);
    right_equal = PatternElementPredicateAccessor(handler, left_and_right[1]);
}
#include "PredicateRuleReader.hpp"

PredicateRuleReader::PredicateRuleReader(PredicateHandler predicate_handler)
{
    // the predicate handler will definitely need to know the predicate classes, and as such should be assigned the taks of reading off predicate.txt
};

PredicateFormationRule PredicateRuleReader::TryReadpredicateRule(string predicate_rule)
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

        predicate_creators.push_back(PredicateCreator(predicate_handler, formation_tokens));
    }

    return PredicateFormationRule(predicate_creators, predicate_modifiers);
};

PredicateFormationRule::PredicateFormationRule(vector<PredicateCreator> predicate_creators, vector<PredicateModifier> predicate_modifiers)
: predicate_creators(predicate_creators), predicate_modifiers(predicate_modifiers) {};

PredicateCreator::PredicateCreator(PredicateHandler handler, vector<string> creation_tokens)
{
    string predicate_name = creation_tokens[0];

    PredicateTemplate parameter_names = handler.GetPredicateTemplate(predicate_name);
    auto parameter_tokens = vector<string>(creation_tokens.begin() + 1, creation_tokens.end());    

    for (int i = 0; i < parameter_tokens.size(); i++)
    {
        string parameter_token = parameter_tokens[i];

        if (parameter_token == "!")
        {
            parameter_creation_types.push_back(ParameterCreationType::WILDCARD);
            continue;
        }

        if (parameter_token.contains_string("->")) // TODO add to string_operators
        {
            parameter_creation_types.push_back(ParameterCreationType::FRAME_PREDICATE_PROPERTY);

            frame_predicate_accessors.push_back(FramePredicateAccessor(handler, parameter_token));
            continue;
        }

        if (parameter_tokens.contains_string("."))
        {
            parameter_creation_types.push_back(ParameterCreationType::WORD_FRAME);
            continue;
        }

        throw invalid_argument("predicate rule string does not match a defined type of predicateFormer's parameter type");
    }
}

FramePredicateAccessor::FramePredicateAccessor(PredicateHandler handler, string token)
{
    vector<string> parts = token.split_string("->"); //TODO add to string_operators
    // verify that the split is correct 
    assert(parts.size() == 2);

    predicate_name = parts[0];
    // verify that predicate is actual rpedicate

}
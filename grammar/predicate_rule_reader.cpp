#include "predicate_rule_reader.hpp"

ParameterCreationType determine_type(string argument)
{
    if (argument.at(0) == '"' && argument.at(argument.size()-1) == '"')
    {
        return ParameterCreationType::STRING;
    }
    if (is_string_all_chars(argument, '!'))
    {
        return ParameterCreationType::WILDCARD;
    }
    else if (find_in_string(argument, (string)"->"))
    {
        return ParameterCreationType::FRAME_PREDICATE_PROPERTY;
    }
    {
        return ParameterCreationType::WORD_FRAME;
    }

    throw runtime_error("bad type of parameter assignment");
}

PredicateRuleReader::PredicateRuleReader(PredicateHandler *predicate_handler) : predicate_handler(predicate_handler)
{ };

PredicateFormationRules::PredicateFormationRules()
{
    predicate_creators = vector<PredicateCreator>();
    predicate_modifiers = vector<PredicateModifier>();
}

PredicateFormationRules::PredicateFormationRules(vector<PredicateCreator> predicate_creators, vector<PredicateModifier> predicate_modifiers)
: predicate_creators(predicate_creators), predicate_modifiers(predicate_modifiers)
{
    // identify the wildcard count for the total expression, so that their names can be easily generated in advance
};

PredicateCreator::PredicateCreator(){
}

PredicateCreator::PredicateCreator(PredicateHandler *handler, vector<string> creation_tokens)
{
    string predicate_name = creation_tokens[0];
    wildcard_list = vector<string>();

    auto parameter_tokens = vector<string>(creation_tokens.begin() + 1, creation_tokens.end());    

    PredicateTemplate predicate_template = PredicateTemplate();
    if (!handler->try_get_predicate_template(predicate_name, &predicate_template))
        throw runtime_error("could not find predicate template for name '"+predicate_name+"'");
    
    int template_size = predicate_template.parameter_names.size(); 
    int token_size = parameter_tokens.size(); 
    if (template_size != token_size)
    {
        // if (template_size > token_size){
        //     string extra_param = predicate_template.parameter_names[template_size];
        //     throw runtime_error("template parameter size "+to_string(template_size)+" does not match parameter token size "+to_string(token_size)+" for predicate "+predicate_name+". First extra parameter is:"+extra_param);

        // } else {
        //     if (template_size > token_size){
        //     string extra_param = par[template_size];
        //     throw runtime_error("template parameter size "+to_string(template_size)+" does not match parameter token size "+to_string(token_size)+" for predicate "+predicate_name+". First extra parameter is:"+extra_param);
        // }
        // }
        printf("fatal break during grammar reading. dumping creation tokens for predicate creator: [");
        for (string token : creation_tokens)
        {
            printf("'%s', ", token.c_str());
        }
        printf("]\n");
        throw runtime_error("template parameter size "+to_string(template_size)+" does not match parameter token size "+to_string(token_size)+" for predicate "+predicate_name+".");
    }

    if (predicate_template.parameter_names.size() != parameter_tokens.size())
        throw runtime_error("wrong size of parameter tokens");

    predicate = predicate_template;
    int template_parameter_index = 0;
    parameter_creation_types = vector<ParameterCreationType>();
    for (int i = 0; i < parameter_tokens.size(); i++)
    {
        string parameter_token = parameter_tokens[i];

        auto parameter_split_colon = split_character(parameter_token, ":");
        if (parameter_split_colon.size() != 2)
            throw runtime_error("parameter entry in predicate creation rule has problem with ':'");
        
        string argument = parameter_split_colon[1];

        string parameter_name = parameter_split_colon[0];

        string corresponding_template_param = predicate.parameter_names[i];

        if (!equals(parameter_name, corresponding_template_param))
            throw runtime_error("parameter name "+parameter_name+" does not match corresponding '"+corresponding_template_param+"' template parameter name");

        if (parameter_name != predicate_template.parameter_names[i])
            throw runtime_error("parameter name in predicate creation rule is named wrong");

        auto param_creation_type = determine_type(argument);
        
        parameter_creation_types.push_back(param_creation_type);

        bool matched = false;
        // store parameter contents
        switch(param_creation_type)
        {
            case WILDCARD:
                matched = true;
                wildcard_list.push_back(argument);
                break;
            case WORD_FRAME:
                matched = true;
                word_frame_accessors.push_back(argument);
                break;
            case FRAME_PREDICATE_PROPERTY:
                matched = true;
                pattern_predicate_accessors.push_back(PatternElementPredicateAccessor(handler, argument));
                break;
            case STRING:
                matched = true;
                param_strings.push_back(trim_front_and_back(argument));
                break;
        }
        if (!matched)
            throw runtime_error("predicate rule string does not match a defined type of predicateFormer's parameter type");
    }

    if (parameter_creation_types.size() != token_size)
        throw runtime_error("parameter creation does not match");
}

PatternElementPredicateAccessor::PatternElementPredicateAccessor() {}

PatternElementPredicateAccessor::PatternElementPredicateAccessor(PredicateHandler* handler, string token)
{
    vector<string> parts = split_character(token, "->");
    // verify that the split is correct 

    assert(parts.size() == 2);

    syntax_frame_name = parts[0];
    // verify that predicate is actual pedicate
    // handler->try_get_predicate_template(predicate_name);


    string leftover_token = parts[1];
    vector<string> predicate_and_parameter = split_character(leftover_token, ".");
    assert(predicate_and_parameter.size() == 2);

    predicate_name = predicate_and_parameter[0];
    parameter_name = predicate_and_parameter[1];
}

string PatternElementPredicateAccessor::stringify()
{
    return syntax_frame_name + "->" + predicate_name + "." + parameter_name;
}

PredicateModifier::PredicateModifier()
{
    
}

PredicateModifier::PredicateModifier(PredicateHandler* handler, string token)
{
    auto left_and_right = split_character(token, "=");

    left_equal = PatternElementPredicateAccessor(handler, left_and_right[0]);
    string right_string = left_and_right[1];
    right_type = determine_type(right_string);

    switch(right_type)
    {
        case FRAME_PREDICATE_PROPERTY:
            right_frame_predicate_property = PatternElementPredicateAccessor(handler, right_string);
            break;
        case WORD_FRAME:
        case WILDCARD:
            right_equal_string = right_string;
            break;
        case STRING:
            right_equal_string = trim_front_and_back(right_string);
            break;
    }
}

bool PredicateRuleReader::try_read_predicate_rule(string predicate_rule, PredicateFormationRules* formation_rules)
{
    auto predicate_creators = vector<PredicateCreator>();
    auto predicate_modifiers = vector<PredicateModifier>();
    auto predicate_types_to_destroy = vector<string>();
    vector<string> predicate_formation_strings = split_character(predicate_rule, "|");

    for (int i = 0; i < predicate_formation_strings.size(); i++)
    {
        string predicate_formation_string = predicate_formation_strings[i];

        trim(predicate_formation_string);
        vector<string> formation_tokens = split_spaces(predicate_formation_string);

        // determine if formation tokens belong to a creator or modifier
        if (predicate_formation_string.find('=') != -1)
        {
            predicate_modifiers.push_back(PredicateModifier(predicate_handler, predicate_formation_string));

            continue;
        }

        if (predicate_formation_string.at(0) == '%')
        {
            predicate_types_to_destroy.push_back(predicate_formation_string.substr(1));
        }

        // ACTION actionVariable:! actor:NounPhrase->IS_INSTANCE_OF.object action:Verb
        predicate_creators.push_back(PredicateCreator(predicate_handler, formation_tokens));
    }

    formation_rules->predicate_creators = predicate_creators;
    // for (int i = 0; i < predicate_creators.size(); i++)
    // {
    //     formation_rules->predicate_creators.at(i) = predicate_creators[i];
    // }

    // formation_rules->predicate_creators = vector<PredicateCreator>(predicate_creators.begin(), predicate_creators.end());
    formation_rules->predicate_modifiers = predicate_modifiers;
    formation_rules->predicate_types_to_destroy = predicate_types_to_destroy;
    return true;
};

// WordFrameAccessor::WordFrameAccessor(string token) : frame_type_name(token)
// {
//     // frame_type_name = token;
//     // if(!handler->(token, &predicate_template))
//     // {

//     //     throw "could not create word frame accessor";

//     // }
// }
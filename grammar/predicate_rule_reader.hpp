#ifndef PREDICATE_RULE_READER
#define PREDICATE_RULE_READER

#include "../logic/predicate_handler.hpp"
// #include "frames.hpp"

enum ParameterCreationType
{
    WILDCARD = 0,
    WORD_FRAME = 1,
    FRAME_PREDICATE_PROPERTY = 2,
    STRING = 3,
};

ParameterCreationType determine_type(string argument);
class PatternElementPredicateAccessor
{
public:
    PatternElementPredicateAccessor();
    PatternElementPredicateAccessor(PredicateHandler *handler, string token);
    //SyntaxFrame->PREDICATE_NAME.parameterName
    string syntax_frame_name;
    string predicate_name;
    string parameter_name;

    // string access_parameter(pair<Frame, Frame> frames, string frame_name);
    string stringify();
};

// class WordFrameAccessor
// {
// public:
//     WordFrameAccessor(string token);

//     // PredicateTemplate predicate_template;
//     string frame_type_name;
//     // string accessed_parameter;

//     // Frame access_word_frame(Expression expression);
// };


class PredicateModifier
{
public:
    PatternElementPredicateAccessor left_equal;
    PatternElementPredicateAccessor right_frame_predicate_property;
    string right_equal_string;

    ParameterCreationType right_type;

    //PrepPhrase->PREPOSITION.action=VerbPhrase->IS_INSTANCE_OF.object
    //SyntaxFrame->PREDICATE_NAME.parameterName=SyntaxFrame->PREDICATE_NAME.parameterName
    //PredicateAccessor=PredicateAccessor
    PredicateModifier();

    PredicateModifier(PredicateHandler* handler, string input_string);

    Expression modify_expression(Expression expression);
};

class PredicateCreator
{
    // ACTION actionVariable:! actor:NounPhrase->IS_INSTANCE_OF.object action:Verb.frame_name
public:
    PredicateTemplate predicate;
    vector<ParameterCreationType> parameter_creation_types;

    vector<string> word_frame_accessors;
    vector<string> param_strings;
    vector<PatternElementPredicateAccessor> pattern_predicate_accessors;
    vector<string> wildcard_list;

    PredicateCreator();
    PredicateCreator(PredicateHandler *handler_ptr, vector<string> creation_tokens);

    // Predicate create_predicate(vector<Predicate> pattern_element_accessor_fodder, vector<Frame> word_frame_accessor_fodder);
};

class PredicateFormationRules
{
public:
    vector<string> predicate_types_to_destroy;
    vector<PredicateCreator> predicate_creators;
    vector<PredicateModifier> predicate_modifiers;

    PredicateFormationRules();
    PredicateFormationRules(vector<PredicateCreator> predicate_creators, vector<PredicateModifier> predicate_modifiers);
};

class PredicateRuleReader
{
private:
    PredicateHandler *predicate_handler;
public:
    PredicateRuleReader(PredicateHandler *handler_ptr);

    bool try_read_predicate_rule(string predicate_rule, PredicateFormationRules *formation_rule);
    
};

#endif
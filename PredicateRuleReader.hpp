#ifndef PREDICATE_RULE_READER
#define PREDICATE_RULE_READER

#include <predicate_handler.hpp>
#include <string_operators.hpp>

class PredicateRuleReader
{
private:
    PredicateHandler predicate_handler;
public:
    PredicateRuleReader(PredicateHandler handler);

    PredicateFormationRule TryReadpredicateRule(string predicate_rule);
    
};

class PredicateFormationRule
{
public:
    vector<PredicateCreator> predicate_creators;
    vector<PredicateModifier> predicate_modifiers;

    PredicateFormationRule(vector<PredicateCreator> predicate_creators, vector<PredicateModifier> predicate_modifiers);
};

class PredicateCreator
{
public:
    PredicateTemplate predicate;
    vector<ParameterCreationType> parameter_creation_types;
    vector<FramePredicateAccessor> frame_predicate_accessors;

    PredicateCreator(PredicateHandler handler, vector<string> creation_tokens);
};

class PredicateModifier
{
    FramePredicateAccessor left_equal;
    FramePredicateAccessor right_equal;

    //PrepPhrase->PREPOSITION.action=VerbPhrase->IS_INSTANCE_OF.object
    //SyntaxFrame->PREDICATE_NAME.parameterName=SyntaxFrame->PREDICATE_NAME.parameterName
    //PredicateAccessor=PredicateAccessor
    PredicateModifier(string input_string);
};

enum ParameterCreationType
{
    WILDCARD = 0,
    WORD_FRAME = 1,
    FRAME_PREDICATE_PROPERTY = 2
};


class FramePredicateAccessor
{
public:
    FramePredicateAccessor(PredicateHandler handler, string token);
    //SyntaxFrame->PREDICATE_NAME.parameterName
    string syntax_frame_name;
    string predicate_name;
    string parameter_name;
};

#endif
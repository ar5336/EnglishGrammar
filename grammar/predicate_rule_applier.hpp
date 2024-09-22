#ifndef GRAMMAR_UTIL
#define GRAMMAR_UTIL

#include "frames.hpp"

#include "../logic/predicate_handler.hpp"

#include "variable_namer.hpp"

class RuleApplierContext
{
public:
    RuleApplierContext(PredicateHandler* predicate_handler, VariableNamer* variable_namer);

    PredicateHandler* predicate_handler;
    VariableNamer* variable_namer;
};

class PredicateRuleApplier
{
private:
    
    static string get_argument_accessor(
        RuleApplierContext context,
        vector<Frame> frames,
        PatternElementPredicateAccessor accessor);

    static bool try_get_predicate(
        RuleApplierContext context,
        vector<Frame> frames,
        PatternElementPredicateAccessor accessor,
        Predicate& result_predicate);
    
    static Expression set_argument_accessor(
        RuleApplierContext context,
        Expression expression,
        vector<Frame> frames,
        PatternElementPredicateAccessor argument_accessor,
        string operand_variable);

    static bool try_get_word_frame(
        RuleApplierContext context,
        string accessor,
        vector<Frame> frames,
        Frame &word_frame);

public:
    static Expression apply_formation_rules_on_expression(
        RuleApplierContext context,
        PredicateFormationRules formation_rule,
        Expression expression,
        vector<Frame> frames);


};

#endif
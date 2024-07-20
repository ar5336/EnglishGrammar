#ifndef PREDICATE_HANDLER_HPP
#define PREDICATE_HANDLER_HPP

#include <string>
#include <map>
#include <utility>
#include <set>

#include "predicate.hpp"
#include "string_operators.hpp"
#include "predicate_template.hpp"
// #include "predicate_rule_reader.hpp"

enum KnowledgeType
{
    GIVEN,
    INFERRED,
};

enum ResponseType
{
    YES,
    NO,
};

// const vector<string> predicate_strings
// {
//     "NONE",
//     "IS_SUBSET_OF",
//     "IS_INSTANCE_OF",
//     "HAS_PROPERTY",
//     "CAN_DO",
// };

class Expression
{
public:
    vector<Predicate> predicates;

    Expression();

    Expression(vector<Predicate> predicates); 

    static Expression combine_expressions(Expression expression1, Expression expression2);

    static Predicate get_predicate_by_name(Expression expression, string predicate_name);

    // removes the predicate from the original expression
    Predicate extract_predicate(Predicate original);

    string stringify();
};

bool operator<(const Expression& lhs, const Expression& rhs);

class PredicateHandler
{
private:
    map<string, string> inheritance_map;
    set<string> noun_set;
    map<string, vector<Expression>> mentioned_nouns;
    // map<string, vector<Predicate>> entity_to_predicate_map;

    map<string, int> predicate_type_map;

    set<Expression> given_expressions;
    set<Expression> inferred_expressions;

    void UpdateInheritanceMap();

    vector<string> IdentifyAllParents(string entityName);

    string StringifyPredicate(Predicate predicate);

public:
    PredicateTemplateHandler *predicate_template_handler;

    vector<pair<KnowledgeType, Expression>> expressions;

    ResponseType DetermineResponse(Expression query_expression);

    void tell(Expression expression);

    PredicateHandler(PredicateTemplateHandler *predicate_template_reader);

    void InferExpressions();

    // void AddPredicate(string type, vector<string> arguments);

    int PredIntFromString(string type);

    Predicate PredFromString(string input);

    bool try_get_predicate_template(string predicate_name, PredicateTemplate *predicate_template);

    Predicate ConstructPredicate(string predicate_name, vector<string> predicate_arguments);
};

#endif
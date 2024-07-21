#ifndef PREDICATE_HANDLER_HPP
#define PREDICATE_HANDLER_HPP

#include <string>
#include <map>
#include <utility>
#include <set>

#include "predicate.hpp"
#include "string_operators.hpp"

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
};

bool operator<(const Expression& lhs, const Expression& rhs);

class PredicateHandler
{
private:

    vector<string> predicate_type_names;

    // map<string, vector<Predicate>> entity_to_predicate_map;

    // map<string, int> predicate_type_map;

public:
    PredicateTemplateHandler *predicate_template_handler;

    string TypeToString(int type_id);

    int StringToTypeId(string string);

    PredicateHandler(PredicateTemplateHandler *predicate_template_reader);

    // void AddPredicate(string type, vector<string> arguments);

    int PredIntFromString(string type);

    Predicate PredFromString(string input);

    bool try_get_predicate_template(string predicate_name, PredicateTemplate *predicate_template);

    Predicate ConstructPredicate(string predicate_name, vector<string> predicate_arguments);

    // string StringifyPredicate(Predicate predicate);

    void init_stringification();

    string stringify_expression(Expression expression);

    string stringify_predicate(Predicate predicate);
};

#endif
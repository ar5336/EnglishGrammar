#ifndef PREDICATE_HANDLER_HPP
#define PREDICATE_HANDLER_HPP

#include <string>
#include <map>
#include <utility>
#include <set>

#include "predicate.hpp"

#include "../string_operators.hpp"
#include "../global.hpp"

class Expression
{
private:
    //prids: predicate ids
    vector<string> mentioned_variables;

    map<string, vector<int>> prids_by_type;
    // instead of a map of variables to variables. A map of predicate to predicate by way of argument name.
    map<int, map<int, tuple<string, string>>> prid_to_prid_by_arg;

    // map<string, map<string, vector<int>>> variable_by_prid_connection_map;

    void make_connections();

    void add_connection(int prid_1, string var_1, int prid_2, string var_2);

    tuple<bool, vector<tuple<int, int>>> has_connection(string pred_1, string arg_1, string pred_2, string arg_2);

public:
    set<string> noun_set;

    vector<Predicate> predicates;

    Expression();

    Expression(vector<Predicate> predicates);

    vector<Predicate> get_predicates();

    // void add_predicate(Predicate predicate);

    // removes the predicate from the original expression
    Predicate extract_predicate(Predicate original);

    static vector<Predicate> extract_predicate_types(Expression& og_expression, set<string> predicate_types);

    // TODO - restrict this to connective arguments not things like noun_class and object_count
    // afterwards, rename to extract_predicates_by_connection
    static vector<Predicate> extract_predicates_by_argument(Expression& og_expression, string argument, bool anaphorics_prohibited);

    static vector<Predicate> extract_anaphora_closure_by_argument(Expression& og_expression, string argument);

    vector<pair<Predicate, Predicate>> get_connections(
        string source_predicate_type,
        string source_argument,
        string target_predicate_type,
        string target_argument);

    static Expression combine_expressions(Expression expression1, Expression expression2);

    static bool try_get_predicate_by_name(Expression expression, string predicate_name, Predicate& result_predicate);

    Predicate operator [](int i) const;
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

    PredicateHandler(PredicateTemplateHandler *predicate_template_reader);

    string type_to_string(int type_id);

    int string_to_type_id(string string);

    int pred_int_from_string(string type);

    Predicate pred_from_string(string input);

    bool try_get_predicate_template(string predicate_name, PredicateTemplate *predicate_template);

    Predicate construct_predicate(string predicate_name, vector<string> predicate_arguments);

    void init_stringification();

    string stringify_expression(Expression expression);

    string stringify_predicate(Predicate predicate);
};

#endif
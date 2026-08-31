#ifndef INFERENCE_RULE_HPP
#define INFERENCE_RULE_HPP

#include <vector>
#include "inference_predicate.hpp"
#include "../../grammar/predicate_rule_reader.hpp"
// #include "../../expression.hpp"

using namespace std;

class InferenceRule
{
private:
// the predicates that must be present for the rule to apply
    PredicateHandler* predicate_handler;
    bool has_all_connected_predicate_matcher;
    bool has_all_connected_predicate_creator;

    // bool do_conditions_apply(Expression basis);
public:
    std::string name;
    
    vector<PredicateMatcher> condition_template;
    Expression condition_as_expression;
    vector<PredicateCreator> conclusion_template;
    
    InferenceRule(
        PredicateHandler* predicate_handler,
        const string& name,
        const vector<PredicateMatcher>& condition_template,
        const vector<PredicateCreator>& conclusion_template);
};

// class InferenceMap
// {
// private:
//     PredicateHandler* predicate_handler;

//     // source_prid -> target_prid pair(source_arg, target_arg)
//     map<int, pair<int, pair<string, string>>> prid_connection_map;

//     InferenceMap(PredicateHandler* predicate_handler);
// }

class PredicateArgumentAddress
{
public:
    PredicateArgumentAddress(
        int predicate_index,
        int statement_index,
        string predicate_type,
        string argument_name)
        : predicate_index(predicate_index),
        statement_index(statement_index),
        predicate_type(predicate_type),
        argument_name(argument_name) {}

    int predicate_index;
    int statement_index;
    string predicate_type;
    string argument_name;
};

class InferencePredicateConnections
{
    // source predicate type -> target predicate type -> (source argument name, target argument name)
public:
    map<PredicateArgumentAddress, set<PredicateArgumentAddress>> source_predicate_connection;
    void add_connection(
        const PredicateArgumentAddress& source,
        const PredicateArgumentAddress& target)
    {
        source_predicate_connection[source].insert(target);
    }

    set<PredicateArgumentAddress> get_connections(
        const PredicateArgumentAddress& source) const
    {
        auto it = source_predicate_connection.find(source);
        if (it != source_predicate_connection.end())
        {
            return it->second;
        }
        else
        {
            return set<PredicateArgumentAddress>();
        }
    }
};

bool operator<(const PredicateArgumentAddress& lhs, const PredicateArgumentAddress& rhs);

class InferenceHandler
{
private:
    PredicateHandler* predicate_handler;

    InferencePredicateConnections existing_predicate_connections;

    // rule name ->
    //   source predicate type -> target predicate type -> (source arg_name, target arg_name)
    map<string, InferencePredicateConnections> inference_rule_predicate_connections;

    // map<string, map<int, int>> identified_conclusion_distance_layers;

    InferencePredicateConnections identify_condition_connections(Expression condition_expression);
public:
    map<string, InferenceRule> inference_rules_by_name;

    InferenceHandler();
    InferenceHandler(vector<InferenceRule> inference_rules, PredicateHandler* predicate_handler);

    void add_inference_rule(const InferenceRule& rule);

    // pair of Inferred Expression, and name of rule used in inference
    vector<pair<Expression, string>> apply_inference_rules(Expression knowledge_base);
};

#endif

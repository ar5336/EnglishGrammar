#ifndef MIND_HPP
#define MIND_HPP

// #include <string>
#include <map>
#include <utility>
#include <set>
#include <vector>
// #include <tuple>
// #include <stack>

#include "predicate_handler.hpp"

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

class Mind
{
private:

    PredicateHandler *predicate_handler;

    map<string, string> child_to_parent_map;
    map<string, vector<string>> ability_map;
    // inheritance map
    // map<string, string> parent_to_child;
    // map<string, string> child_to_parent;

    set<string> noun_set;
    map<string, vector<Expression>> mentioned_nouns;

    set<Expression> given_expressions;
    set<Expression> inferred_expressions;
    
    void apply_inheritance_rule(Expression expression);
    void apply_ability_rule(Expression expression);

    vector<string> identify_all_parents(string entityName);

    void make_inferences(Expression expression);
    void update_conceptual_maps(Expression new_expression);

    // dog IS mammal
    Expression construct_subset_expression(string noun_1, string noun_2);
    // cat CAN_DO run
    Expression construct_ability_expression(string noun_1, string action_type);
public:
    Mind();

    void init(PredicateHandler *predicate_handler);

    vector<pair<KnowledgeType, Expression>> expressions;


    void infer(Expression expression);

    ResponseType ask(Expression expression);

    void tell(Expression expression);
};

// class InheritanceGraph
// {
//     vector<InheritanceNode> nodes;
//     map<string, InheritanceNode> nodes_by_name;

//     void add_node(InheritanceNode new_node);
// };

// class InheritanceNode
// {
// public:
//     set<string> parents;
//     set<string> children;
// }

#endif
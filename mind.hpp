#ifndef MIND_HPP
#define MIND_HPP

// #include <string>
// #include <map>
#include <utility>
#include <set>
#include <vector>
// #include <tuple>
#include <stack>

#include "predicate_handler.hpp"

enum KnowledgeType
{
    GIVEN,
    INFERRED,
};

// enum ResponseType
// {
//     YES,
//     NO,
// };

class ConceptualEntity
{
public:
    string noun;

    set<string> parents;
    set<string> children;

    vector<string> ability_action_types;
    vector<string> properties;

    ConceptualEntity(string noun);
};

class ConceptualSchema
{
private:

    // returns child to parent pairings indicated by expression
    vector<pair<string, string>> extract_inheritances(Expression expression);

    // returns nount to action pairings indicated by expression
    vector<pair<string, string>> extract_abilities(Expression expression);

    void apply_inheritance_rule(Expression expression);
    void apply_ability_rule(Expression expression);
    //void apply_activity_mentioned_rule(Expression expression);

    void update_inheritances(string child, string parent);
    void update_abilities(string noun, string ability, int recursion = 1);

    void print_maps();

    bool can_do(string noun, string action);
    void add_ability(string noun, string action);
public:
    vector<ConceptualEntity> nouns;

    set<string> noun_set;
    
    map<string, ConceptualEntity> entities_by_noun;
    map<string, set<string>> child_to_parents_map;
    map<string, set<string>> parent_to_children_map;

    map<string, set<string>> ability_map;

    ConceptualSchema();

    bool has_noun(string noun);

    set<string> get_parents(string noun);

    // bool try_apply_expression(Expression applicant);

    void add_entity(ConceptualEntity new_node);

    void consider_expression(Expression expression);

    void update_conceptual_maps(Expression new_expression);

    pair<bool, string> try_resolve_inquisitive_expression(Expression expression);

    // void make_inferences(Expression expression);
};

class Mind
{
private:

    PredicateHandler *predicate_handler;
    ConceptualSchema *conceptual_schema;

    // map<string, string> child_to_parent_map;
    // inheritance map
    // map<string, string> parent_to_child;
    // map<string, string> child_to_parent;

    // set<string> noun_set;
    map<string, vector<Expression>> mentioned_nouns;

    set<Expression> given_expressions;
    set<Expression> inferred_expressions;

    // vector<string> identify_all_parents(string entityName);

    // dog IS mammal
    Expression construct_subset_expression(string noun_1, string noun_2);
    // cat CAN_DO run
    Expression construct_ability_expression(string noun_1, string action_type);

public:
    Mind(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema);

    // void init(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema);

    vector<pair<KnowledgeType, Expression>> expressions;

    string ask(Expression expression);

    void tell(Expression expression);
};

// TODO - create
// ConceptualEntityConnections and
// ConceptualEntityAbility and
// ConceptualEntityProperty classes for thorough information connecting conceptual entities

#endif
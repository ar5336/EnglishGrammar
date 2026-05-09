#ifndef CONCEPTUAL_SCHEMA_HPP
#define CONCEPTUAL_SCHEMA_HPP

#include <utility>
#include <vector>
#include <stack>

#include "expression.hpp"

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

    // returns noun to action pairings indicated by expression
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
    vector<ConceptualEntity> nouns_classes;

    set<string> noun_class_set;
    
    map<string, ConceptualEntity> entities_by_noun;
    map<string, set<string>> child_to_parents_map;
    map<string, set<string>> parent_to_children_map;

    map<string, set<string>> ability_map;

    ConceptualSchema();

    bool has_noun(string noun);

    ConceptualEntity get_noun_entity(string noun);

    set<string> get_parents(string noun);

    // bool try_apply_expression(Expression applicant);

    void add_entity(ConceptualEntity new_node);

    void consider_expression(Expression expression);

    void update_conceptual_maps(Expression new_expression);

    pair<bool, string> try_resolve_inquisitive_expression(Expression expression);

    // void make_inferences(Expression expression);
};

#endif
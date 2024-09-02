#ifndef MIND_HPP
#define MIND_HPP

// #include <string>
// #include <map>
#include <utility>
// #include <set>
// #include <map>
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

// class NounProfile
// {
// public:
//     ConceptualEntity *entity_type;

//     int id;
//     string name;
//     set<string> properties;

//     int current_best_concrete_match;
//     vector<int> participations;

//     bool does_it_still_match();
// };

class Noun
{
    // vector<int> noun_profile_ids;
public:
    ConceptualEntity *entity_type;

    int id;
    string name;

    // set<string> properties_from_class;
    set<string> properties;

    // set<ActionIndicator> particular_actions;
    // vector<Event> relevant_events;

    // Point2i location;
    // Point2f size;

    Noun(string name, ConceptualEntity* entity_type, int id, bool real);

    string stringify();
    bool real;
};

class Event
{
private:

    string location;

    int id;

    // this can be like,
    // attempted
    // set<string> features;

public:
    string action_type;
    // these can not stay as strings of noun_class
    // they should point to ConceptualEntity
    string actor_noun_class;
    int actor_noun_id;

    string subject_noun_class;
    int subject_noun_id;

    Event();

    // constructor for actualized entity
    Event(
        string action_type,
        string actor_noun_class,
        int actor_noun_id,
        string subject_noun_class,
        int subject_noun_id,
        int id);

    Event(
        string action_type,
        string actor_noun_class,
        int actor_noun_id,
        int id);

    string stringify();

    bool has_subject();
    
    bool has_actor();

    bool real;
};

class Timeline
{
private:
    

public:
    vector<Event> actions;

    // how will these locations on the timeline be referenced? bucketing.

    // void emplace_action(
    //     string location,
    //     string pivot,
    //     string direction // forwars, backward
    // );

    Timeline();
    Timeline(bool real);

    bool real;
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

    vector<Event> extract_events(Expression expression, bool real);

    vector<pair<int, string>> extract_names();

    map<int, vector<string>> extract_concrete_properties(Expression expression);

    int create_new_object(Predicate is_predicate, bool real = true);

    Expression resolve_properties(Expression expression);

    Noun* dereference_noun_id(int noun_id, bool real);

    bool compare_events(Event event_1, Event event_2);

    int id_counter = 0;
public:
    Mind(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema);

    // void init(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema);

    vector<pair<KnowledgeType, Expression>> expressions;

    string ask(Expression expression);

    void tell(Expression expression);

    vector<Noun> concrete_nouns;
    vector<Noun> abstract_nouns;

    Timeline timeline;
    Timeline abstract_timeline;

    Expression resolve_anaphoras(Expression expression);

    bool did_it_occur(Event event, Event& og_event);


    // void create_concrete_noun(Noun noun);
};

// TODO - create
// ConceptualEntityConnections and
// ConceptualEntityAbility and
// ConceptualEntityProperty classes for thorough information connecting conceptual entities

#endif
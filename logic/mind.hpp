#ifndef MIND_HPP
#define MIND_HPP

#include <utility>
#include <vector>
#include <stack>

#include "conceptual_schema.hpp"
#include "expression.hpp"
#include "inference/inference_rule.hpp"

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

    string indirect_noun_class;
    int indirect_noun_id;

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
        string subject_noun_class,
        int subject_noun_id,
        string indirect_noun_class,
        int indirect_noun_id,
        int id);

    Event(
        string action_type,
        string actor_noun_class,
        int actor_noun_id,
        int id);

    string stringify();

    bool has_subject();
    
    bool has_actor();

    bool has_indirect();

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

    vector<pair<int, int>> causality_map;

    Timeline();
    Timeline(bool real);

    bool real;
};

class Mind
{
private:

    PredicateHandler *predicate_handler;
    ConceptualSchema *conceptual_schema;
    InferenceHandler *inference_handler;

    // map<string, string> child_to_parent_map;
    // inheritance map
    // map<string, string> parent_to_child;
    // map<string, string> child_to_parent;

    // set<string> noun_set;
    map<string, vector<Expression>> mentioned_nouns;

    set<Expression> given_expressions;
    set<Expression> inferred_expressions;

    // void apply_to_connections(Expression expression, function<void(Predicate, Predicate)> func);

    // vector<string> identify_all_parents(string entityName);

    vector<Event> extract_events(Expression expression, bool real);

    vector<pair<int, string>> extract_names();

    vector<pair<int, pair<string, string>>> extract_prepositions();

    vector<pair<string, string>> extract_properties(Expression expression);

    map<int, vector<string>> extract_concrete_properties(Expression expression);

    int create_object_representation(Predicate is_predicate, bool real = true);

    Expression resolve_properties(Expression expression);

    Expression resolve_anaphoras(Expression expression);

    Noun& dereference_noun_id(int noun_id, bool real);

    bool compare_events(Event event_1, Event event_2);

    int id_counter = 0;
    
public:
    Mind(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema, InferenceHandler* inference_handler);

    // void init(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema);

    vector<pair<KnowledgeType, Expression>> expressions;

    string ask(Expression expression);

    void tell(Expression expression);

    vector<Noun> concrete_nouns;
    vector<Noun> abstract_nouns;

    Timeline timeline;
    Timeline abstract_timeline;

    bool does_it_exist(Noun noun, Noun& og_noun);
    bool did_it_occur(Event event, Event& og_event);
};

// TODO - create
// ConceptualEntityConnections and
// ConceptualEntityAbility and
// ConceptualEntityProperty classes for thorough information connecting conceptual entities

#endif
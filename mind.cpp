#include "mind.hpp"

Mind::Mind(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema)
    : predicate_handler(predicate_handler), conceptual_schema(conceptual_schema)
{
    timeline = Timeline();
}

void Mind::tell(Expression expression)
{
    if ((given_expressions.emplace(expression)).second)
    {
        expressions.push_back(pair<KnowledgeType, Expression>(KnowledgeType::GIVEN, expression));
    }
    
    conceptual_schema->consider_expression(expression);

    // resolve anaphoric references (eventually move this to a WorldModel object along with event extraction)
    expression = resolve_anaphoras(expression);

    // properties must be resolved after anaphoras. as anaphoras determine which objects are defined or not
    expression = resolve_properties(expression);
    if (DEBUGGING)
    {
        printf("finished resolving anaphoras\n");
        printf("%s\n", predicate_handler->stringify_expression(expression).c_str());
    }

    for (auto event : extract_events(expression, false))
    {
        timeline.actions.push_back(event);

        // add the uninitialized noun instances to the concrete nouns list
        string actor_noun_class = event.actor_noun_class;
        string subject_noun_class = event.subject_noun_class;

        bool is_actor_initialized = event.has_actor();
        bool create_actor_noun = false;
        if (is_actor_initialized && concrete_nouns.size() <= event.actor_noun_id)
        {
            // throw runtime_error ("unknown noun class '" + actor_noun_class + "'\n");
            create_actor_noun = true;
        }

        bool is_subject_initialized = event.has_subject();
        bool create_subject_noun = false;
        if (is_subject_initialized && concrete_nouns.size() <= event.subject_noun_id)
        {
            // throw runtime_error ("unknown noun class '" + subject_noun_class + "'\n");
            create_subject_noun = true;
        }

        if (DEBUGGING)
        {
            if (!create_actor_noun)
                printf("\033[1;32mcreating\033[0m concrete actor noun '%s'\n", actor_noun_class.c_str());
            if (!create_subject_noun)
                printf("\033[1;32mcreating\033[0m concrete subject noun '%s'\n", subject_noun_class.c_str());
        }

        if (is_actor_initialized && create_actor_noun)
            concrete_nouns.push_back(ConcreteNoun("unknown", &conceptual_schema->entities_by_noun.at(actor_noun_class), id_counter++));
        
        if (is_subject_initialized && create_subject_noun)
            concrete_nouns.push_back(ConcreteNoun("unknown", &conceptual_schema->entities_by_noun.at(subject_noun_class), id_counter++));
    }
}

enum ActionParamType
{
    NONE,
    ACTOR,
    SUBJECT,
    SUBJECT_2,
};

Expression Mind::resolve_anaphoras(Expression expression)
{
    if (DEBUGGING)
    {
        printf("resolving anaphoras\n");
    }
    auto stripped_expression = expression;
    // lump together all the predicates that are attached to the anaphoric noun
    // gather all the predicates that reference the anaphoric variable
    vector<Predicate> anaphoric_predicates = Expression::extract_predicate_types(stripped_expression, {"ANAPHORIC"});

    vector<vector<Predicate>> anaphora_groups;
    vector<Event> resolved_anaphora_events;
    vector<Predicate> action_predicates;
    
    for (auto anaphoric_predicate : anaphoric_predicates)
    {
        if (DEBUGGING)
        {
            printf("resolving anaphora\n");
        }

        string base_var_name = anaphoric_predicate.get_argument("object");
        string anaphoric_var_name = anaphoric_predicate.get_argument("anaphoric_object");

        // extract all predicates from the og expression that mention the anaphoric variable
        vector<Predicate> anaphora_group = Expression::extract_anaphora_closure_by_argument(stripped_expression, base_var_name);

        // identify the events described in the anaphora
        Expression anaphora_expression = Expression(anaphora_group);
        // conceptual_schema->consider_expression(anaphora_expression)
        // introduce some sort of call here where the
        // IS statemetnts are transformed into concrete object references
        // - to simplify things greatly down the line
        auto events = extract_events(anaphora_expression, false);

        if (DEBUGGING)
        {
            printf("number of events in anaphora: %ld\n", events.size());
            printf("anaphora expression: %s\n", predicate_handler->stringify_expression(anaphora_expression).c_str());
        }

        vector<Predicate> action_predicates_extracted = Expression::extract_predicate_types(anaphora_expression, {"ACTION_2", "ACTION"});
        if (action_predicates_extracted.size() > 1)
            throw runtime_error("more than one event per anaphora, parsing not implemented for this yet");

        action_predicates.push_back(action_predicates_extracted.at(0));

        if (events.size() != 1)
        {   
            //TODO - change the behavior hrere, as in the future, anaphoras will be able to refer to concrete nouns, or even abstract nouns
            throw runtime_error("failed to resolve anaphora into event. found " + to_string(events.size()) + " events.");
        }

        resolved_anaphora_events.push_back(events.at(0));
    }

    vector<Predicate> restored_expression_preds = stripped_expression.predicates;

    for (int i = 0; i < resolved_anaphora_events.size(); i++)
    {
        Event event = resolved_anaphora_events.at(i);
        auto anaphoric_predicate = anaphoric_predicates.at(i);

        string base_var_name = anaphoric_predicate.get_argument("object");
        string anaphoric_var_name = anaphoric_predicate.get_argument("anaphoric_object");

        ActionParamType param_type = ActionParamType::NONE;

        // identify which ACTION_2 param type of base_var_name
        Predicate action_predicate = action_predicates.at(i);
        if (equals(action_predicate.get_argument("actor"), base_var_name))
            param_type = ActionParamType::ACTOR;
        
        if (equals(action_predicate.predicate_template.predicate, "ACTION_2"))
            if (equals(action_predicate.get_argument("object"), base_var_name))
                param_type = ActionParamType::SUBJECT;

        Event pass_event = Event();
        if (timeline.did_it_occur(event, pass_event))
        {
            if (DEBUGGING)
            {
                printf("anaphora resolution underway\n");
            }

            Event relevant_event = event;

            vector<string> args = vector<string> {anaphoric_var_name};

            if (param_type == ActionParamType::ACTOR)
                args.push_back(to_string(pass_event.actor_noun_id));
            else if (param_type == ActionParamType::SUBJECT)
                args.push_back(to_string(pass_event.subject_noun_id));
            else 
                throw runtime_error("unsupported anaphoric reference to an argument in an action predicate");

            // IS object:a object_count:1 noun_class:fish       IS object:a object_count:1 noun_class:fish
            // ANAPHORIC object:a                           =>  OBJECT object:a id:45  (maps to a fish named billy or something)
            restored_expression_preds.push_back(
                predicate_handler->construct_predicate(
                    "OBJECT",
                    args
                )
            );
        }
    }

    return Expression(restored_expression_preds);
}

string Mind::ask(Expression expression)
{
    // simple yes/no as of now
    // auto asStatement = Predicate(queryPredicate.type_id, queryPredicate.arguments);

    // first check if the statement resolves into existing entity inheritance/ability information from Conceptual Schema
    auto resolution_pair = conceptual_schema->try_resolve_inquisitive_expression(expression);

    bool is_resolved = resolution_pair.first;
    string resolution_message = resolution_pair.second;

    if (is_resolved)
    {
        return resolution_message;
    }

    // next, resolve against events
    auto events = extract_events(expression, true);
    for (auto event : events)
    {
        Event pass_event = Event();
        if (timeline.did_it_occur(event, pass_event))
        {
            return "yes, it did happen";
        }
    }

    if (events.size() != 0)
    {
        return "no, it did not happen";
            }

    // can't do a simple hash comparison now. need to check connection equivalence between expression
    return "unknown";
}

ConcreteNoun* Mind::dereference_noun_id(int noun_id)
{
    if (concrete_nouns.size() <= noun_id)
        throw runtime_error("id would cause sgmentation fault. noun id: " + to_string(noun_id) + ", concrete nouns size: " + to_string(concrete_nouns.size()));
    
    return &concrete_nouns.at(noun_id);
}

void ConceptualSchema::print_maps()
{
    // print out the child_to_parents_map
    printf("CHILD TO PARENTS MAP\n");
    for (auto map_entry : child_to_parents_map)
    {
        string child = map_entry.first;
        set<string> parents = map_entry.second;

        printf("%s => [", child.c_str());
        for (string parent : parents)
        {
            printf("%s,", parent.c_str());
        }
        printf("]\n");
    }

    printf("PARENT TO CHILDREN MAP\n");
    for (auto map_entry : parent_to_children_map)
    {
        string parent = map_entry.first;
        set<string> children = map_entry.second;

        printf("%s => [", parent.c_str());
        for (string child : children)
        {
            printf("%s,", child.c_str());
        }
        printf("]\n");
    }
}

vector<pair<string, string>> ConceptualSchema::extract_inheritances(Expression expression)
{
    // IS (3) -> CONTAINS -> IS

    auto constructed_response = vector<pair<string, string>>();

    auto connection_pairs = expression.get_connections(
        "IS", "object",
        "CONTAINS", "container");

    if (connection_pairs.size() == 0)
        return constructed_response;

    for (auto connection_pair : connection_pairs)
    {
        Predicate is_predicate = connection_pair.first;

        if (!equals(is_predicate.get_argument("object_count"), "inf"))
        {
            // don't do this rule anymore
            return constructed_response;
        }

        Predicate contains_predicate = connection_pair.second;

        // auto parent_child_pairs = vector<tuple<string, string>>();

        auto second_connection_pairs = expression.get_connections(
            "CONTAINS", "containee",
            "IS", "object");

        if (second_connection_pairs.size() == 0)
            return constructed_response;
        
        for (auto second_connection_pair : second_connection_pairs)
        {
            Predicate other_is_predicate = second_connection_pair.second;

            string parent = other_is_predicate.get_argument("noun_class");
            string child = is_predicate.get_argument("noun_class");

            constructed_response.push_back(make_pair(parent, child));
            
            // child_to_parent.emplace(child, parent);
            // parent_child_pairs.push_back(make_tuple(parent, child));
        }
    }

    return constructed_response;
}

void ConceptualSchema::update_conceptual_maps(Expression new_expression)
{
    if (DEBUGGING)
        printf("\033[1;33mupdating\033[0m conceptual maps\n");
    
    apply_inheritance_rule(new_expression);
    apply_ability_rule(new_expression);
}

// returns bool is_resolved and a string message
pair<bool, string> ConceptualSchema::try_resolve_inquisitive_expression(Expression expression)
{
    // for now, only simple inheritance questions - "are dogs mammals", "are dogs animals"

    // first, check if the nouns_classes mentioned are defined in the schema
    auto expression_noun_set = expression.noun_set;
    for (auto expression_noun : expression_noun_set)
    {
        if (noun_class_set.size() == 0)
        {
            if (DEBUGGING)
                printf("no defined nouns\n");
            return make_pair(false, "no known nouns");
        }

        if (!has_noun(expression_noun))
        {   
            return make_pair(false, "noun '"+expression_noun+"' is not a known noun");
        }
    }

    if (DEBUGGING)
        printf("mentioned variables are known\n");

    // then, find what inheritance this expression indicates
    vector<pair<string, string>> inheritances = extract_inheritances(expression);

    bool constructed_bool = true;
    string constructed_response = "";

    if (DEBUGGING)
    {
        print_maps();
    }

    // build the response string as you consider each of the indicated inheritances
    if (DEBUGGING)
        printf("number of inheritances found: %ld\n", inheritances.size());

    if (inheritances.size() > 0)
    {
        for (auto inheritance : inheritances)
        {
            string child = inheritance.first;
            string parent = inheritance.second;

            if (child_to_parents_map.count(child) == 0)
            {
                constructed_bool &= false;
                constructed_response += ("no, "+child+" does not inherit from "+parent+".");
                continue;
            }
            set<string> potential_parent_matches = child_to_parents_map.at(child);

            if (potential_parent_matches.find(parent) != potential_parent_matches.end())
            {
                constructed_response += ("yes, "+child+" inherits from "+parent+".");
            } else {

                constructed_response += ("no, "+child+" does not inherit from "+parent+".");
            }
        }
        return make_pair(true, constructed_response);
    }
    

    vector<pair<string, string>> abilities = extract_abilities(expression);

    if (abilities.size() > 0)
    {
        for (auto ability_pair : abilities)
        {
            string noun = ability_pair.first;
            string ability = ability_pair.second;

            if (DEBUGGING)
                printf("checking if %s can do  ability:'%s'", noun.c_str(), ability.c_str());

            if (can_do(noun, ability))
            {
                return make_pair(true, "yes, "  + noun + " can " + ability + ".");
            }
            else
            {
                return make_pair(true, "no, " + noun + " can not " + ability + ".");
            }
        }
    }
 

    return make_pair(false, "");
}

template <typename T>
std::set<T> getUnion(const std::set<T>& a, const std::set<T>& b)
{
  std::set<T> result = a;
  result.insert(b.begin(), b.end());
  return result;
}

ConceptualSchema::ConceptualSchema()
{
    noun_class_set = set<string>();
}

bool ConceptualSchema::has_noun(string noun)
{
    return noun_class_set.find(noun) != noun_class_set.end();
}

void ConceptualSchema::add_entity(ConceptualEntity new_node)
{
    if (DEBUGGING)
    {
        printf("\033[1;32madding\033[0m object of noun class '%s'\n", new_node.noun.c_str());
    }
    entities_by_noun.emplace(new_node.noun, new_node);
}

void ConceptualSchema::consider_expression(Expression expression)
{
    auto expression_nouns = expression.noun_set;

    set<string> new_nouns = set<string>();

    for (string expression_noun : expression_nouns)
    {
        if (!has_noun(expression_noun))
        {
            if (DEBUGGING)
                printf("added new noun '%s' to noun_class_set\n", expression_noun.c_str());
            // if expression noun is not present in current nouns
            noun_class_set.insert(expression_noun);
            add_entity(ConceptualEntity(expression_noun));
        }
    }

    update_conceptual_maps(expression);
    // make_inferences(expression);
}

void ConceptualSchema::apply_ability_rule(Expression expression)
{
    if (DEBUGGING)
        printf("applying ability rule\n");

    auto noun_ability_pairs = extract_abilities(expression);

    if (DEBUGGING && noun_ability_pairs.size() > 0)
        printf("%d abilities found in expression\n", (int)noun_ability_pairs.size());

    for (auto noun_ability_pair : noun_ability_pairs)
    {
        string actor_arg = noun_ability_pair.first;
        string ability_arg = noun_ability_pair.second;

        if (ability_map.count(actor_arg) == 0)
        {
            add_ability(actor_arg, ability_arg);

            update_abilities(actor_arg, ability_arg);
        }
        else {
            set<string> current_abilities = ability_map.at(actor_arg);
            if (current_abilities.count(ability_arg) == 0)
            {
                add_ability(actor_arg, ability_arg);
                ability_map.at(actor_arg).emplace(ability_arg);
                update_abilities(actor_arg, ability_arg);
            }
            // can already do ability
        }

    }

}

void ConceptualSchema::add_ability(string noun, string action)
{
    if (DEBUGGING)
        printf("\033[1;32madding\033[0m ability '%s' to noun '%s'\n", action.c_str(), noun.c_str());

    if (ability_map.count(noun) == 0)
    {
        ability_map.emplace(noun, set<string>{action});
    } else {
        ability_map.at(noun).emplace(action);
    }
}


vector<pair<string, string>> ConceptualSchema::extract_abilities(Expression expression)
{
    auto actor_ability_pairs = expression.get_connections(
        "IS", "object",
        "CAN_DO", "actor");

    if (DEBUGGING)
        printf("extracted %d abilities\n", (int)actor_ability_pairs.size());

    vector<pair<string, string>> noun_ability_pairs = vector<pair<string, string>>();
    for (auto actor_ability_pair : actor_ability_pairs)
    {
        Predicate actor_predicate = actor_ability_pair.first;
        Predicate ability_predicate = actor_ability_pair.second;

        string actor_arg = actor_predicate.get_argument("noun_class");
        string ability_arg = ability_predicate.get_argument("action_type");

        noun_ability_pairs.push_back(make_pair(actor_arg, ability_arg));

    }

    return noun_ability_pairs;
}

Event::Event()
{
    actor_noun_class = "";
    subject_noun_class = "";
    actor_noun_id = 1;
    subject_noun_id = 1;
    id = -1;
    action_type = "unknown";
}

Event::Event(
    string action_type,
    string actor_noun_class,
    int actor_noun_id,
    string subject_noun_class,
    int subject_noun_id,
    int id) 
    : action_type(action_type),
    actor_noun_class(actor_noun_class),
    actor_noun_id(actor_noun_id),
    subject_noun_class(subject_noun_class),
    subject_noun_id(subject_noun_id),
    id(id)
{
    location = "unknown";
}

Event::Event(
    string action_type,
    string actor_noun_class,
    int actor_noun_id,
    int id) 
    : action_type(action_type),
    actor_noun_class(actor_noun_class),
    actor_noun_id(actor_noun_id),
    id(id)
{
    subject_noun_class = "unknown";
    subject_noun_id = -1;
    location = "unknown";
}

string Event::stringify()
{
    string constructee = "";
    constructee += "Transitive Event [" + to_string(id) + "]:\n";
    constructee += "    Action type: " + action_type + "\n";
    if (has_actor())
        constructee += "    Actor: " + actor_noun_class + "[" + to_string(actor_noun_id) + "]\n"; 
    if (has_subject())
        constructee += "    Subject: " + subject_noun_class + "[" + to_string(subject_noun_id) + "]\n";
    return constructee;
}

bool Event::has_subject()
{
    return !equals(subject_noun_class, "unknown");
}

bool Event::has_actor()
{
    return !equals(actor_noun_class, "unknown");
}

bool unknown_or_match(string arg_1, string arg_2)
{
    return (equals(arg_1, "unknown")
     || equals(arg_2, "unknown")
     || equals(arg_1, arg_2));
}

bool Event::compare(Event event_1, Event event_2)
{
    if (!equals(event_1.action_type, event_2.action_type))
        return false;
    
    if (!unknown_or_match(event_1.actor_noun_class, event_2.actor_noun_class))
        return false;

    if (!unknown_or_match(event_1.subject_noun_class, event_2.subject_noun_class))
        return false;
    
    return true;
}

vector<Event> Mind::extract_events(Expression expression, bool modify_nouns = false)
{
    vector<Event> identified_events = vector<Event>();

    auto object_event_pairs = expression.get_connections(
        "IS", "object",
        "ACTION", "actor");

    for (auto object_event_pair : object_event_pairs)
    {
        Predicate actor_predicate = object_event_pair.first;
        Predicate action_predicate = object_event_pair.second;

        auto event = Event(
            action_predicate.get_argument("action_type"), // action_type
            actor_predicate.get_argument("noun_class"),       // actor
            (int)concrete_nouns.size(),
            timeline.actions.size());

        auto og_event = Event();
        if (timeline.did_it_occur(event, og_event))
            identified_events.push_back(Event(
                action_predicate.get_argument("action_type"), // action_type
                og_event.actor_noun_class,       // actor
                og_event.actor_noun_id,
                timeline.actions.size()));
        else
        {
            identified_events.push_back(event);
        }
    }

    if (identified_events.size() != 0)
        return identified_events;

    object_event_pairs = expression.get_connections(
        "IS", "object",
        "ACTION_2", "actor");

    auto concrete_object_event_pairs = expression.get_connections(
        "OBJECT", "object",
        "ACTION_2", "actor");
    
    for (auto pair : concrete_object_event_pairs)
    {
        object_event_pairs.push_back(pair);
    }
    
    Event new_event = Event();
    for (auto object_event_pair : object_event_pairs)
    {
        new_event = Event();

        Predicate actor_predicate = object_event_pair.first;
        Predicate action_predicate = object_event_pair.second;

        bool is_actor_concrete = equals(actor_predicate.predicate_template.predicate, "OBJECT");

        auto event_other_object_pairs = expression.get_connections(
            "ACTION_2", "object",
            "IS", "object");

        auto other_other_object_event_pairs = expression.get_connections(
            "ACTION_2", "object",
            "OBJECT", "object");

        for (auto pair : other_other_object_event_pairs)
        {
            event_other_object_pairs.push_back(pair);
        }
        
        for (auto event_other_object_pair : event_other_object_pairs)
        {
            Predicate object_predicate = event_other_object_pair.second;

            bool is_actor_concrete = equals(actor_predicate.predicate_template.predicate, "OBJECT");
            bool is_object_concrete = equals(object_predicate.predicate_template.predicate, "OBJECT");
            
            printf("before the storm\n");
            int actor_id = is_actor_concrete ? stoi(actor_predicate.get_argument("id")) : (int)concrete_nouns.size();
            string actor_noun_class = is_actor_concrete ? concrete_nouns[actor_id].entity_type->noun : actor_predicate.get_argument("noun_class");

            printf("eye of the storm\n");
            int object_id = is_object_concrete ? stoi(object_predicate.get_argument("id")) : (int)concrete_nouns.size() + 1;
            string object_noun_class = is_object_concrete ? concrete_nouns[object_id].entity_type->noun : object_predicate.get_argument("noun_class");
            printf("after the storm\n");

            new_event = Event(
                action_predicate.get_argument("action_type"), // action_type
                actor_noun_class,       // actor
                actor_id,
                object_noun_class,
                object_id,
                timeline.actions.size());
            identified_events.push_back(new_event);   // subject
        }
    }

    if (identified_events.size() != 0)
        return identified_events;

    auto event_other_object_pairs = expression.get_connections(
            "ACTION_2", "object",
            "IS", "object");

    if (event_other_object_pairs.size() <= 0)
        return identified_events;

    for (auto event_other_object_pair : event_other_object_pairs)
    {
        Predicate action_predicate = event_other_object_pair.first;
        Predicate subject_predicate = event_other_object_pair.second;

        new_event = Event(
            action_predicate.get_argument("action_type"), // action_type
            "unknown",       // actor
            -1,
            subject_predicate.get_argument("noun_class"),
            (int)concrete_nouns.size(),
            timeline.actions.size());

        identified_events.push_back(new_event);   // subject
    }

    return identified_events;
}

vector<pair<int, string>> Mind::extract_properties()
{
    return vector<pair<int, string>>();
}

int Mind::create_new_object(Predicate is_predicate)
{
    if (!is_predicate.has_argument("noun_class"))
    {
        printf("\033[1;31error: failed to create new object with predicate basis of: %s\033[0m", predicate_handler->stringify_predicate(is_predicate).c_str());
    }
    string noun_class = is_predicate.get_argument("noun_class");
    int size = concrete_nouns.size();
    concrete_nouns.push_back(ConcreteNoun(
        "unknown",
        &conceptual_schema->entities_by_noun.at(noun_class),
        size
    ));
    return size;
}

Expression Mind::resolve_properties(Expression expression)
{
    Expression modified_expression = expression;

    // first, check object
    auto object_property_pairs = expression.get_connections(
        "OBJECT", "object",
        "HAS_PROPERTY", "object");

    for (auto object_property_pair : object_property_pairs)
    {
        Predicate object_predicate = object_property_pair.first;
        Predicate property_predicate = object_property_pair.second;

        int object_noun_id = stoi(object_predicate.get_argument("id"));

        string property = property_predicate.get_argument("property");

        if (DEBUGGING)
            printf("adding property %s to object id %d\n", property.c_str(), object_noun_id);

        concrete_nouns.at(object_noun_id).properties.emplace(property);
    }

    // then check is
    auto is_property_pairs = expression.get_connections(
        "IS", "object",
        "HAS_PROPERTY", "object");

    for (auto is_property_pair : is_property_pairs)
    {
        Predicate is_predicate = is_property_pair.first;
        Predicate property_predicate = is_property_pair.second;

        printf("is predicate: %s\n", predicate_handler->stringify_predicate(is_predicate).c_str());
        printf("property predicate: %s\n", predicate_handler->stringify_predicate(property_predicate).c_str());

        int object_noun_id = create_new_object(is_predicate);

        string property = property_predicate.get_argument("property");

        if (DEBUGGING)
            printf("adding property %s to object id %d\n", property.c_str(), object_noun_id);

        concrete_nouns.at(object_noun_id).properties.emplace(property);

        // replace the IS with the OBJECT in the original expression
        modified_expression.extract_predicate(is_predicate);
        modified_expression.predicates.push_back(predicate_handler->construct_predicate("OBJECT", {is_predicate.get_argument("object"), to_string(object_noun_id)}));
    }

    return modified_expression;
}

void ConceptualSchema::apply_inheritance_rule(Expression expression)
{
    int LOOP_LIMIT = 3;

    if (DEBUGGING)
        printf("applying inheritance rule\n");

    auto inheritances = extract_inheritances(expression);

    // add the map connections
    for (auto inheritance : inheritances)
    {
        auto child = inheritance.first;
        auto parent = inheritance.second;

        if (child_to_parents_map.count(child) == 0)
        {
            child_to_parents_map.emplace(child, set<string>{parent});
        }
        else 
        {
            child_to_parents_map.at(child).emplace(parent);
        }

        if (parent_to_children_map.count(parent) == 0)
        {
            parent_to_children_map.emplace(parent, set<string>{child});
        }
        else
        {
            parent_to_children_map.at(parent).emplace(child);
        }

        if (DEBUGGING)
            printf("new inheritance added from child:'%s' to parent:'%s'\n", child.c_str(), parent.c_str());

        update_inheritances(child, parent);
    }
    
    return;

    // for each new parent child pair, create the 
}

void ConceptualSchema::update_inheritances(string child, string parent)
{
    // noun is expected to have recieved a new parent, update all of its children
    // using the child to parents and parent to chlidren map, update them both.
    if (DEBUGGING)
        printf("updating inheritance maps for child '%s' and parent '%s'\n", child.c_str(), parent.c_str());
    
    if (DEBUGGING)
    {
        printf("the maps as they are whilst updating:\n");
        print_maps();
    }

    // TODO - put a update_abilities() call in some places here.
    // for the case of | mammals are animals, animals can swim, dogs are mammals ==> dogs can swim

    // step 1) make sure all the parents of the parent are inherited to the child
    if (child_to_parents_map.count(parent) != 0)
    {
        // so, in this case it would be that child is cat and parent is animal
        set<string> parents_parents = child_to_parents_map.at(parent);
        for (string grandparent : parents_parents)
        {
            // child: 
            child_to_parents_map.at(child).insert(grandparent);
            parent_to_children_map.at(grandparent).insert(child);
            if (DEBUGGING)
                printf("\033[1;32madding\033[0m grandparent '%s' to parents of child '%s\n", grandparent.c_str(), child.c_str());
        }
    }

    if (parent_to_children_map.count(child) == 0)
        return;
        // throw runtime_error("could not find child '" + child + "' in parent_to_children map");


    //step 2) make sure the parents are inherited to all children
    auto children_visited = set<string>();
    vector<string> children_to_traverse = vector<string>{child};

    while (children_to_traverse.size() != 0)
    {
        string child_to_traverse = children_to_traverse.back();
        children_to_traverse.pop_back();

        auto children_of_child = parent_to_children_map.at(child_to_traverse);

        for (string child_of_child : children_of_child)
        {
            if (child_to_parents_map.count(child_of_child) == 0)
                throw runtime_error("nested child "+child_of_child+" not found in child_to_parent map");
            
            set<string> sub_parents = child_to_parents_map.at(child_of_child);
            
            if (sub_parents.find(parent) == sub_parents.end())
            {
                child_to_parents_map.at(child_of_child).emplace(parent);

                // if this child you just updated the parents of, has children of its own, look at it later.
                if (parent_to_children_map.count(child_of_child) != 0
                    && children_visited.find(child_of_child) == children_visited.end())
                {
                    if (DEBUGGING)
                        printf("\033[1;32madding\033[0m child '%s' to traversal\n", child_of_child.c_str());
                    children_to_traverse.push_back(child_of_child);
                }
                children_visited.emplace(child_to_traverse);

                if (DEBUGGING)
                {
                    printf("\033[1;32madded\033[0m parent '%s' to child '%s'\n", parent.c_str(), child_of_child.c_str());
                }
            }
        }
    }

    

}

void ConceptualSchema::update_abilities(string noun, string ability, int recursion)
{
    int ABILITY_RECURSION_DEPTH = 4;

    if (recursion > ABILITY_RECURSION_DEPTH)
    {
        printf("WARNING: recursion depth limit reached in update_abilities");
        return;
    }

    if (parent_to_children_map.count(noun) == 0)
    {
        return;
    }

    set<string> children = parent_to_children_map.at(noun);
    
    for (string child : children)
    {
        if (!can_do(child, ability))
        {
            add_ability(child, ability);
            update_abilities(child, ability, recursion + 1);
        }
    }
}

bool ConceptualSchema::can_do(string noun, string action)
{
    if (DEBUGGING)
        printf("checking if noun '%s' can do action '%s'\n", noun.c_str(), action.c_str());
    
    return (ability_map.count(noun) != 0) &&
        (ability_map.at(noun).count(action) != 0);
}

ConceptualEntity::ConceptualEntity(string noun) : noun(noun)
{
    noun = "NOTHING";
}

Timeline::Timeline()
{
    actions = vector<Event>();
}

bool Timeline::did_it_occur(Event event, Event& og_event)
{
    // TODO - make this capable of handling multiple matches
    for (auto occurence : actions)
    {
        if (Event::compare(occurence, event))
        {
            og_event = occurence;
            return true;
        }
    }
    return false;
}

ConcreteNoun::ConcreteNoun(string name, ConceptualEntity *entity_type, int id)
    : name(name), entity_type(entity_type), id(id)
{
}

string ConcreteNoun::stringify()
{
    string str = "";
    str += "Object:\n";
    str += "  id: " + to_string(id) + "\n";
    str += "  noun class: " + entity_type->noun + "\n";
    str += "  name: " + name + "\n";
    str += "  properties: [" + stringify_set(properties) + "]\n";
    return str;
}

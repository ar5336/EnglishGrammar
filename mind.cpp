#include "mind.hpp"

Mind::Mind(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema)
    : predicate_handler(predicate_handler), conceptual_schema(conceptual_schema)
{
    timeline = Timeline();
    abstract_timeline = Timeline(false);
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
    if (DEBUGGING)
    {
        printf("finished resolving anaphoras\n");
        printf("%s\n", predicate_handler->stringify_expression(expression).c_str());
    }
    expression = resolve_properties(expression);

    auto events = extract_events(expression, true);
    for (Event event : events)
    {
        timeline.actions.push_back(event);
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
        if (did_it_occur(event, pass_event))
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

    expression = resolve_anaphoras(expression);

    auto properties_map = extract_concrete_properties(expression);

    if (DEBUGGING)
        printf("extracted %ld properties\n", properties_map.size());

    bool props_match = false;
    for (auto id_to_props : properties_map)
    {
        int ob_id = id_to_props.first;
        vector<string> props = id_to_props.second;
        for (auto prop : props)
        {
            if (concrete_nouns.at(ob_id).properties.count(prop) == 0)
                return "no, it does not have the property '" + prop + "'";
            else
                props_match = true;
        }
    }

    if (props_match)
        return "yes, it does have that property";
    
    // next, resolve against events
    auto events = extract_events(expression, false);
    for (auto event : events)
    {
        Event pass_event = Event();
        if (DEBUGGING)
            printf("checking if event: \n%s\n occurred", event.stringify().c_str());
    
        if (did_it_occur(event, pass_event))
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

Noun* Mind::dereference_noun_id(int noun_id, bool real = true)
{
    if (real)
    {
        int cs = concrete_nouns.size();
        if (noun_id < 0 || cs <= noun_id)
            throw runtime_error("id would cause sgmentation fault. noun id: " + to_string(noun_id) + ", concrete nouns size: " + to_string(concrete_nouns.size()));
    }
    else
    {
        int as = abstract_nouns.size();
        if (noun_id < 0 || as <= noun_id)
            throw runtime_error("id would cause sgmentation fault. noun id: " + to_string(noun_id) + ", abstract nouns size: " + to_string(abstract_nouns.size()));
    }
    if (real)
    {
        return &concrete_nouns.at(noun_id);
    }
    return &abstract_nouns.at(noun_id);
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
    entities_by_noun = map<string, ConceptualEntity>();
}

bool ConceptualSchema::has_noun(string noun)
{
    return noun_class_set.count(noun) != 0;
}

ConceptualEntity ConceptualSchema::get_noun_entity(string noun)
{
    if (!has_noun(noun))
        throw runtime_error("noun of class name \'" + noun + "\' not found in conceptual schema");

    return entities_by_noun.at(noun);
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
    real = true;
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
    real = true;
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
    real = true;
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

bool do_args_accord(string arg_abstract, string arg_concrete)
{
    return (equals(arg_abstract, "unknown")
     || equals(arg_abstract, arg_concrete));

     // TODO - add special case for an unknown concrete.
     //     A> something bit my fish
     //     B> did a horse bite your fish?
     //     A> i dunno maybe
}

bool compare_nouns(Noun abstract_noun, Noun concrete_noun)
{
    for (auto property : abstract_noun.properties)
    {
        if (concrete_noun.properties.count(property) == 0)
        {
            return false;
        }
    }

    if (!do_args_accord(abstract_noun.name, concrete_noun.name))
        return false;
    return true;
}

bool Mind::compare_events(Event event_in_question, Event concrete_event)
{
    if (DEBUGGING)
        printf("\033[1;34mcomparing events\n%s\nand\n%s\033[0m\n", event_in_question.stringify().c_str(), concrete_event.stringify().c_str());
    
    if (!equals(event_in_question.action_type, concrete_event.action_type))
        return false;
    
    if (!do_args_accord(event_in_question.actor_noun_class, concrete_event.actor_noun_class))
        return false;

    if (!do_args_accord(event_in_question.subject_noun_class, concrete_event.subject_noun_class))
        return false;
    
    if (event_in_question.actor_noun_id != -1)
    {
        Noun abstract_noun = abstract_nouns[event_in_question.actor_noun_id];
        Noun concrete_noun = concrete_nouns[concrete_event.actor_noun_id];
        
        if (!compare_nouns(abstract_noun, concrete_noun))
            return false;
    }

    if (event_in_question.subject_noun_id != -1)
    {
        Noun abstract_noun = abstract_nouns[event_in_question.subject_noun_id];
        Noun concrete_noun = concrete_nouns[concrete_event.subject_noun_id];
        
        if (!compare_nouns(abstract_noun, concrete_noun))
            return false;
    }

    
    return true;
}

vector<Event> Mind::extract_events(Expression expression, bool real = true)
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
            create_new_object(actor_predicate, real),
            real ? timeline.actions.size() : abstract_timeline.actions.size());

        identified_events.push_back(event);
    }

    object_event_pairs = expression.get_connections(
        "OBJECT", "object",
        "ACTION", "actor");

    for (auto object_event_pair : object_event_pairs)
    {
        Predicate actor_predicate = object_event_pair.first;
        Predicate action_predicate = object_event_pair.second;

        int actor_id = stoi(actor_predicate.get_argument("id"));
        auto event = Event(
            action_predicate.get_argument("action_type"), // action_type
            dereference_noun_id(actor_id, real)->entity_type->noun,       // actor
            actor_id,
            real ? timeline.actions.size() : abstract_timeline.actions.size());

        identified_events.push_back(event);
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
            
            if (DEBUGGING)
            {
                printf("beginning object creation and dereferencing\n");
                printf("actor predicate: %s\n", predicate_handler->stringify_predicate(actor_predicate).c_str());
            }
            
            int actor_id = is_actor_concrete ? stoi(actor_predicate.get_argument("id")) : create_new_object(actor_predicate, real);
            string actor_noun_class = is_actor_concrete ? dereference_noun_id(actor_id, real)->entity_type->noun : actor_predicate.get_argument("noun_class");

            int object_id = is_object_concrete ? stoi(object_predicate.get_argument("id")) : create_new_object(object_predicate, real);
            string object_noun_class = is_object_concrete ? dereference_noun_id(actor_id, real)->entity_type->noun : object_predicate.get_argument("noun_class");
            
            if (DEBUGGING)
                printf("ending object creation and dereferencing\n");

            new_event = Event(
                action_predicate.get_argument("action_type"), // action_type
                actor_noun_class,       // actor
                actor_id,
                object_noun_class,
                object_id,
                real ? timeline.actions.size() : abstract_timeline.actions.size());
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
            create_new_object(subject_predicate, real),
            real ? timeline.actions.size() : abstract_timeline.actions.size());

        identified_events.push_back(new_event);   // subject
    }

    return identified_events;
}

int Mind::create_new_object(Predicate is_predicate, bool real)
{
    if (DEBUGGING)
        printf("creating %s object for predicate %s\n", real ? "real" : "abstract", predicate_handler->stringify_predicate(is_predicate).c_str());

    if (!is_predicate.has_argument("noun_class"))
    {
        printf("\033[1;31error: failed to create new object with predicate basis of: %s\033[0m", predicate_handler->stringify_predicate(is_predicate).c_str());
    }

    string noun_class = is_predicate.get_argument("noun_class");

    int size = -1;
    
    if (real)
        size = concrete_nouns.size();
    else
        size = abstract_nouns.size();

    if (!conceptual_schema->has_noun(noun_class))
    {
        throw runtime_error("noun of class name \'" + noun_class + "\' not found in conceptual schema");
    }

    Noun noun = Noun(
        "unknown",
        &conceptual_schema->entities_by_noun.at(noun_class) ,
        size,
        real
    );

    if (real)
    {
        concrete_nouns.push_back(noun);
        if (DEBUGGING)
            printf("adding real noun\n");
    }
    else 
    {
        abstract_nouns.push_back(noun);
        if (DEBUGGING)
            printf("adding abstract noun\n");
    }
    return size;
}

map<int, vector<string>> Mind::extract_concrete_properties(Expression expression)
{
    auto property_pairs = map<int, vector<string>>();

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

        if (property_pairs.count(object_noun_id) != 0)
            property_pairs.at(object_noun_id).push_back(property);
        else
            property_pairs.emplace(object_noun_id, vector<string> {property});
    }

    return property_pairs;
}

Expression Mind::resolve_properties(Expression expression)
{

    // TODO - extract some of this logic into extract_properties. (the extraction part)
    // and have that be used in the anaphora and tell methods.
    // for anaphora, to chekc the did_it_occur correctly,
    // and for the tell to make sure the properties of the described events are logged
    // resolve properties should only be used at the end.
    Expression modified_expression = expression;

    // first check is
    auto is_property_pairs = expression.get_connections(
        "IS", "object",
        "HAS_PROPERTY", "object");

    set<Predicate> modified_predicates;
    for (auto is_property_pair : is_property_pairs)
    {
        Predicate is_predicate = is_property_pair.first;

        if (modified_predicates.count(is_predicate) != 0)
            continue;

        Predicate property_predicate = is_property_pair.second;

        if (DEBUGGING)
        {
            printf("is predicate: %s\n", predicate_handler->stringify_predicate(is_predicate).c_str());
            printf("property predicate: %s\n", predicate_handler->stringify_predicate(property_predicate).c_str());
        }

        int object_noun_id = create_new_object(is_predicate);

        string property = property_predicate.get_argument("property");

        if (DEBUGGING)
            printf("adding property %s to object id %d\n", property.c_str(), object_noun_id);

        concrete_nouns.at(object_noun_id).properties.emplace(property);

        // replace the IS with the OBJECT in the original expression
        modified_predicates.emplace(is_predicate);

        modified_expression.extract_predicate(is_predicate);
        modified_expression.predicates.push_back(predicate_handler->construct_predicate("OBJECT", {is_predicate.get_argument("object"), to_string(object_noun_id)}));
    }

    modified_expression = Expression(modified_expression.predicates);

    // then, check object
    auto properties_map = extract_concrete_properties(modified_expression);
    for (auto id_to_props : properties_map)
    {
        int ob_id = id_to_props.first;
        vector<string> props = id_to_props.second;
        for (auto prop : props)
        {
            concrete_nouns.at(ob_id).properties.emplace(prop);
        }
    }
    
    if (DEBUGGING)
        printf("resolved pos-property expression:\n%s\n", predicate_handler->stringify_expression(modified_expression).c_str());
    
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
    : real(true)
{
    actions = vector<Event>();
}

Timeline::Timeline(bool real)
    : real(real)
{
    actions = vector<Event>();
}

bool Mind::did_it_occur(Event abstract_event, Event &og_event)
{
    if (DEBUGGING)
        printf("checking if action:\n%s\n did occur\n", abstract_event.stringify().c_str());
    
    // TODO - make this capable of handling multiple matches
    for (auto concrete_event : timeline.actions)
    {
        if (compare_events(abstract_event, concrete_event))
        {
            og_event = concrete_event;
            return true;
        }
    }
    return false;
}

Noun::Noun(string name, ConceptualEntity *entity_type, int id, bool real)
    : name(name), entity_type(entity_type), id(id), real(real)
{
}

string Noun::stringify()
{
    string str = "";
    str += "Object:\n";
    str += "  id: " + to_string(id) + "\n";
    str += "  noun class: " + entity_type->noun + "\n";
    str += "  name: " + name + "\n";
    str += "  properties: [" + stringify_set(properties) + "]\n";
    return str;
};
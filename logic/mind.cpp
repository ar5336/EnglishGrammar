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

    if (DEBUGGING)
        printf("done resolving anaphora for statemnt\n");

    // properties must be resolved after anaphoras. as anaphoras determine which objects are defined or not
    if (DEBUGGING)
    {
        printf("finished resolving anaphoras\n");
        printf("%s\n", predicate_handler->stringify_expression(expression).c_str());
    }
    expression = resolve_properties(expression);

    auto events = extract_events(expression, true);
    if (DEBUGGING)
        printf("done extracting events for statemnt\n");
    
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
        auto events = extract_events(anaphora_expression, false);

        if (DEBUGGING)
        {
            printf("number of events in anaphora: %ld\n", events.size());
            printf("anaphora expression: %s\n", predicate_handler->stringify_expression(anaphora_expression).c_str());
        }

        vector<Predicate> action_predicates_extracted = Expression::extract_predicate_types(anaphora_expression, {"ACTION"});
        if (action_predicates_extracted.size() > 1)
            printf("zamn\n");
            // throw runtime_error("more than one event per anaphora, parsing not implemented for this yet");

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

        string fetched_arg;
        if (action_predicate.try_get_argument("actor", fetched_arg) && equals(fetched_arg, base_var_name))
            param_type = ActionParamType::ACTOR;

        if (action_predicate.try_get_argument("object", fetched_arg) && equals(fetched_arg, base_var_name))
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

    expression = resolve_properties(expression);
    
    if (events.size() != 0)
    {
        return "no, it did not happen";
    }

    // can't do a simple hash comparison now. need to check connection equivalence between expression
    return "unknown";
}

Noun& Mind::dereference_noun_id(int noun_id, bool real)
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
        return concrete_nouns.at(noun_id);
    }
    return abstract_nouns.at(noun_id);
}



template <typename T>
std::set<T> getUnion(const std::set<T>& a, const std::set<T>& b)
{
  std::set<T> result = a;
  result.insert(b.begin(), b.end());
  return result;
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
    string subject_noun_class,
    int subject_noun_id,
    string indirect_noun_class,
    int indirect_noun_id,
    int id) 
    : action_type(action_type),
    actor_noun_class(actor_noun_class),
    actor_noun_id(actor_noun_id),
    subject_noun_class(subject_noun_class),
    subject_noun_id(subject_noun_id),
    indirect_noun_class(indirect_noun_class),
    indirect_noun_id(indirect_noun_id),
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
    if (has_indirect())
        constructee += "    Indirect Object: " + indirect_noun_class + "[" + to_string(indirect_noun_id) + "]\n";
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

bool Event::has_indirect()
{
    return !equals(indirect_noun_class, "unknown");
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

    auto action_id_to_event_map = map<string, Event>();
    // need to process the ACTION-IS and ACTION-OBJECT in a pair.
    // factor out the processing of different parameters of the ACTION predicate

    PredicateTemplate action_template;
    predicate_handler->predicate_template_handler->try_get_predicate_template("ACTION", &action_template);

    auto action_parameter_names = action_template.parameter_names;
    auto action_schematic_parameters = action_template.are_params_schematic;
    auto action_optional_parameters = action_template.are_params_optional;

    auto action_param_to_noun_class_map = map<string, map<string, string>>();
    auto action_param_to_noun_id_map = map<string, map<string, int>>();

    auto actions_identified = set<string>();
    auto action_to_action_type_map = map<string, string>();

    string reflexive_schematic_param = "action";

    for (int param_i = 0; param_i < action_parameter_names.size(); param_i++)
    {
        // process each parameter
        bool is_schematic = action_schematic_parameters.at(param_i);
        bool is_optional = action_optional_parameters.at(param_i);

        if (!is_schematic)
        {
            continue;
        }

        // process optional parameters
        if (is_optional)
        {
            // handle optional parameter case
        }

        string param_name = action_parameter_names.at(param_i);

        if (equals(param_name, reflexive_schematic_param))
            continue;

        // for now we assume every parameter in the action predicate is an object
        auto connections = expression.get_connections(
            "ACTION", param_name,
            "IS", "object");
        
        // add on connections to OBJECT predicates
        auto object_connections = expression.get_connections(
            "ACTION", param_name,
            "OBJECT", "object");
        
        connections.insert(connections.end(), object_connections.begin(), object_connections.end());

        for (auto connection : connections)
        {
            Predicate param_predicate = connection.second;
            
            bool is_param_concrete = equals(param_predicate.predicate_template.predicate, "OBJECT");
    
            int param_object_id = is_param_concrete
                    ? stoi(param_predicate.get_argument("id"))
                    : create_object_representation(param_predicate, real);
            string actor_noun_class = is_param_concrete
                ? dereference_noun_id(param_object_id, real).entity_type->noun
                : param_predicate.get_argument("noun_class");

            Predicate original_action_predicate = connection.first;
            
            string action_id = original_action_predicate.get_argument(reflexive_schematic_param);
            // add parameter identifications to map
            action_param_to_noun_class_map[action_id][param_name] = actor_noun_class;
            action_param_to_noun_id_map[action_id][param_name] = param_object_id;

            actions_identified.insert(action_id);
            action_to_action_type_map[action_id] = original_action_predicate.get_argument("action_type");
        }
    }

    // traverse maps by list of actions
    for (const auto& action_id : actions_identified)
    {
        auto object_noun_class = action_param_to_noun_class_map[action_id]["actor"];
        auto object_noun_id = action_param_to_noun_id_map[action_id]["actor"];

        auto subject_class = action_param_to_noun_class_map[action_id]["object"];
        auto subject_noun_id = action_param_to_noun_id_map[action_id]["object"];

        auto indirect_class = action_param_to_noun_class_map[action_id]["indirect_object"];
        auto indirect_noun_id = action_param_to_noun_id_map[action_id]["indirect_object"];

        // Create an event for each identified action
        // depending on which params are identified, call different constructors for Event

        Event new_event = Event(
            action_to_action_type_map[action_id],
            object_noun_class,
            object_noun_id,
            subject_class,
            subject_noun_id,
            indirect_class,
            indirect_noun_id,
            real ? timeline.actions.size() : abstract_timeline.actions.size()
        );

        identified_events.push_back(new_event);
    }

    if (DEBUGGING)
        printf("returning identified events");

    return identified_events;
}

vector<pair<int, string>> Mind::extract_names()
{
    return vector<pair<int, string>>();
}

int Mind::create_object_representation(Predicate is_predicate, bool real)
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
    // printf("\033[1;31mwarning\033[0m: this method should never be called\n");
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

// vector<pair<string, string>> Mind::extract_properties(Expression expression)
// {
//     // auto predicate_pairs = expression.get_connections(
//     //     "IS", "object",
//     //     "HAS_PROPERTY", "object");

//     // for (auto object_property_pair : predicate_pairs)
//     // {
//     //     Predicate object_predicate = object_property_pair.first;
//     //     Predicate property_predicate = object_property_pair.second;

//     //     int object_noun_id = stoi(object_predicate.get_argument("id"));

//     //     string property = property_predicate.get_argument("property");

//     //     if (DEBUGGING)
//     //         printf("adding property %s to object id %d\n", property.c_str(), object_noun_id);

//     //     if (property_pairs.count(object_noun_id) != 0)
//     //         property_pairs.at(object_noun_id).push_back(property);
//     //     else
//     //         property_pairs.emplace(object_noun_id, vector<string> {property});
//     // }

//     // return property_pairs;
// }

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

        int object_noun_id = create_object_representation(is_predicate);

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

bool Mind::does_it_exist(Noun noun, Noun& og_noun)
{
    vector<Noun> matches;

    for (auto concrete_noun : concrete_nouns)
        if (!compare_nouns(noun, concrete_noun))
            matches.push_back(concrete_noun);

    if (matches.size() > 1)
        if (DEBUGGING || WARNING)
            printf("\033[1;31mwarning\033[0m: unaccomodated ambiguity in noun reference\n");

    if (matches.size() == 0)
        return false;
    
    og_noun = matches[0];
    return true;
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

// a method that takes in a function pointer and a vector of strings that trace out the path of a set of connections
// and applies the function to each of the connections.

// void Mind::apply_to_connections(
//     Expression expression,
//     function<void(Predicate, Predicate)> func)
// {
//     auto connections = expression.get_connections();

//     for (auto connection : connections)
//     {
//         func(connection.first, connection.second);
//     }
// }
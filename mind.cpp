#include "mind.hpp"

Mind::Mind(PredicateHandler *predicate_handler, ConceptualSchema *conceptual_schema)
    : predicate_handler(predicate_handler), conceptual_schema(conceptual_schema)
{
}

void Mind::tell(Expression expression)
{
    if ((given_expressions.emplace(expression)).second)
    {
        expressions.push_back(pair<KnowledgeType, Expression>(KnowledgeType::GIVEN, expression));
    }

    conceptual_schema->consider_expression(expression);
}

// vector<string> Mind::identify_all_parents(string entity_name)
// {
//     const int WHILE_LIMIT = 100;

//     // TODO - update child_to_parent_map to be <string> => vector<string>
//     // so you can have multiple inheritance
//     vector<string> parent_stack;

//     set<string> visited_entities;
//     string current_entity_name = entity_name;
//     while (conceptual_schema->has_noun(current_entity_name) != 0
//         && visited_entities.count(current_entity_name) == 0)
//     {
//         set<string> parents = conceptual_schema->get_parents(current_entity_name);

//         for (string parent_name : parents)
//         {
//             if (visited_entities.count(parent_name) == 0)
//                 parent_stack.push_back(parent_name);

//             visited_entities.insert(current_entity_name);
//             current_entity_name = parent_name;
//         }

        
//         if (visited_entities.size() > WHILE_LIMIT)
//             throw runtime_error("WHILE_LIMIT reached");
//     }

//     if (DEBUGGING && parent_stack.size() > 1)
//         printf("found parents for child '%s' count: %ld\n", entity_name.c_str(), parent_stack.size());

//     return parent_stack;
// }

string Mind::ask(Expression expression)
{
    // simple yes/no as of now
    // auto asStatement = Predicate(queryPredicate.type_id, queryPredicate.arguments);

    // first check if the statement resolves into existing entity inheritance/ability information from Conceptual Schema
    auto resolution_pair = conceptual_schema->try_resolve_expression(expression);

    bool is_resolved = resolution_pair.first;
    string resolution_message = resolution_pair.second;

    if (is_resolved)
    {
        return resolution_message;
    }

    // TODO - resolve against concrete database.
    // for questions like "is fido the dog brown"

    // can't do a simple hash comparison now. need to check connection equivalence between expression
    return "unknown";

    // auto is_given = given_expressions.count(expression) != 0;
    // auto is_inferred = inferred_expressions.count(expression) != 0;

    // if (is_given || is_inferred)
    // {
    //     return ResponseType::YES;
    // }

    // return ResponseType::NO;
}

Expression Mind::construct_subset_expression(string noun_1, string noun_2)
{
    string object_1_var = "a";
    string object_2_var = "b";

    // construct can_do
    vector<Predicate> predicates = vector<Predicate>();

    predicates.push_back(predicate_handler->construct_predicate(
        "IS",
        vector<string>
        {
            /*object*/ object_1_var,
            /*object_count*/ "3",
            /*noun_class*/ noun_1,
        }
    ));

    predicates.push_back(predicate_handler->construct_predicate(
        "IS",
        vector<string>
        {
            /*object*/ object_2_var,
            /*object_count*/ "1",
            /*noun_class*/ noun_2,
        }
    ));

    predicates.push_back(predicate_handler->construct_predicate(
        "CONTAINS",
        vector<string>
        {
            /*container*/ object_2_var,
            /*containee*/ object_1_var
        }
    ));

    return Expression(predicates);
}

Expression Mind::construct_ability_expression(string noun_1, string action_type)
{
    string object_var = "a";
    string action_var = "b";

    // construct can_do
    vector<Predicate> predicates = vector<Predicate>();

    predicates.push_back(predicate_handler->construct_predicate(
        "IS",
        vector<string>
        {
            /*object*/ object_var,
            /*object_count*/ "3",
            /*noun_class*/ noun_1,
        }
    ));

    predicates.push_back(predicate_handler->construct_predicate(
        "CAN_DO",
        vector<string>
        {
            /*action_type*/ action_type,
            /*actor*/ object_var,
        }
    ));

    return Expression(predicates);
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

    for (auto connection_pair : connection_pairs)
    {
        Predicate is_predicate = connection_pair.first;

        if (!equals(is_predicate.get_argument("object_count"), "3"))
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
        printf("updating conceptual maps\n");
    
    apply_inheritance_rule(new_expression);
    apply_ability_rule(new_expression);
}

// returns bool is_resolved and a string message
pair<bool, string> ConceptualSchema::try_resolve_expression(Expression expression)
{
    // for now, only simple inheritance questions - "are dogs mammals", "are dogs animals"

    // first, check if the nouns mentioned are defined in the schema
    auto expression_noun_set = expression.noun_set;
    for (auto expression_noun : expression_noun_set)
    {
        if (noun_set.size() == 0)
        {
            if (DEBUGGING)
                printf("nouns are empty\n");
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

    if (inheritances.size() == 0)
        return make_pair(false, "no inheritances found");

    bool constructed_bool = true;
    string constructed_response = "";

    if (DEBUGGING)
    {
        print_maps();
    }

    // build the response string as you consider each of the indicated inheritances
    if (inheritances.size() != 0)
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

            // if (parent_to_children_map.count(noun) == 0)
            printf("checking if %s can do  ability:'%s'", noun.c_str(), ability.c_str());

            if (can_do(noun, ability))
            {
                return make_pair(true, "yes, "+noun+" can "+ability+".");
            }
        }
    }
 

    printf("resolved response: %s\n", constructed_response.c_str());
    return make_pair(true, constructed_response);
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
    noun_set = set<string>();
}

bool ConceptualSchema::has_noun(string noun)
{
    return noun_set.find(noun) != noun_set.end();
}

void ConceptualSchema::add_entity(ConceptualEntity new_node)
{
    // just update the children of your children, don't care about parents pointers yet. no need.
    stack<string> children_updating_stack;
    if (new_node.children.size() != 0)
    {
        for (string child : new_node.children)
        {
            children_updating_stack.push(child);
        }
    }

    // traverse to update parents and children
    while (children_updating_stack.size() > 0)
    {
        string child_to_update = children_updating_stack.top();

        if (entities_by_noun.count(child_to_update) == 0)
           throw runtime_error("child of ConceptualEntity not found");

        ConceptualEntity node = entities_by_noun.at(child_to_update);

        node.children = getUnion(node.children, new_node.children);

    }
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
                printf("added new noun '%s' to noun_set\n", expression_noun.c_str());
            // if expression noun is not present in current nouns
            noun_set.insert(expression_noun);
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

    vector<pair<string, string>> noun_ability_pairs;
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
                printf("adding grandparent '%s' to parents of child '%s\n", grandparent.c_str(), child.c_str());
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
                        printf("adding child '%s' to traversal\n", child_of_child.c_str());
                    children_to_traverse.push_back(child_of_child);
                }
                children_visited.emplace(child_to_traverse);

                if (DEBUGGING)
                {
                    printf("added parent '%s' to child '%s'\n", parent.c_str(), child_of_child.c_str());
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
    return (ability_map.count(noun) != 0) &&
        (ability_map.at(noun).count(action) != 0);
}

// void ConceptualSchema::make_inferences(Expression new_expression){
//     // first pass, build inheritance map
//     update_conceptual_maps(new_expression);

//     if (DEBUGGING)
//         printf("making inferences\n");

//     printf("noun set size: %ld\n", noun_set.size());
//     for(string noun : noun_set)
//     {
//         auto parent_list = identify_all_parents(noun);
        
//         // if any of the parents has an ability, then the noun has an ability
//         for (string parent : parent_list) {
//             if (DEBUGGING)
//                 printf("making inference on parent: '%s' of noun '%s'\n", parent.c_str(), noun.c_str());

//             if (ability_map.count(parent) != 0)
//             {
//                 auto inherited_abilities = ability_map.at(parent);

//                 for (string inherited_ability : inherited_abilities)
//                 {
//                     infer(construct_ability_expression(noun, inherited_ability));

//                 }
//             }

//             if (child_to_parents_map.count(parent) != 0)
//             {
//                 auto inherited_parent = child_to_parents_map.at(parent);

//                 infer(construct_subset_expression(noun, inherited_parent));
//             }

//             // if (first_arg_to_predicate_map.count(parent) != 0)
//             // {
//             //     auto parent_predicates = first_arg_to_predicate_map.at(parent);

//             //     for (auto parent_predicate : parent_predicates)
//             //     {
//             //         auto new_tupe = parent_predicate.type;
//             //         auto changed_args = parent_predicate.arguments;
//             //         changed_args[0] = noun;

//             //         // TODO - find a more efficient way of not inferring certain already-inferred things
//             //         auto new_expr = construct_expression(new_tupe, changed_args);
//             //         if (inferred_predicates.count(new_expr) == 0) {
//             //             auto was_inserted = inferred_predicates.insert(new_expr);
//             //             if (was_inserted.second) {
//             //                 expressions.push_back(new_expr);
//             //             }
//             //         }
//             //     }
//             // }
//         }
//     }
// }

ConceptualEntity::ConceptualEntity(string noun) : noun(noun)
{
    noun = "NOTHING";
}

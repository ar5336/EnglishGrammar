#include "mind.hpp"

Mind::Mind() {}

void Mind::init(PredicateHandler *predicate_handler)
{
    predicate_handler = predicate_handler;
}

void Mind::tell(Expression expression)
{
    if ((given_expressions.emplace(expression)).second)
    {
        expressions.push_back(pair<KnowledgeType, Expression>(KnowledgeType::GIVEN, expression));
    }
    make_inferences(expression);
}

vector<string> Mind::identify_all_parents(string entity_name)
{
    // TODO - update child_to_parent_map to be <string> => vector<string>
    // so you can have multiple inheritance
    vector<string> parent_stack;

    set<string> visited_entities;
    string current_entity_name = entity_name;
    while (child_to_parent_map.count(current_entity_name) != 0
        && visited_entities.count(current_entity_name) == 0)
    {
        string parent = child_to_parent_map.at(current_entity_name);
        parent_stack.push_back(parent);
        visited_entities.emplace(current_entity_name);
        current_entity_name = parent;
        
    }
    return parent_stack;
}

void Mind::make_inferences(Expression new_expression){
    // first pass, build inheritance map
    update_conceptual_maps(new_expression);
    return;

    for(string noun : noun_set)
    {
        auto parent_list = identify_all_parents(noun);
        
        // if any of the parents has an ability, then the noun has an ability
        for (string parent : parent_list) {
            printf("making inference on parent: %s", parent.c_str());
            if (ability_map.count(parent) != 0)
            {
                auto inherited_abilities = ability_map.at(parent);

                for (string inherited_ability : inherited_abilities)
                {
                    infer(construct_ability_expression(noun, inherited_ability));

                }
            }

            if (child_to_parent_map.count(parent) != 0)
            {
                auto inherited_parent = child_to_parent_map.at(parent);

                infer(construct_subset_expression(noun, inherited_parent));
            }

            // if (first_arg_to_predicate_map.count(parent) != 0)
            // {
            //     auto parent_predicates = first_arg_to_predicate_map.at(parent);

            //     for (auto parent_predicate : parent_predicates)
            //     {
            //         auto new_tupe = parent_predicate.type;
            //         auto changed_args = parent_predicate.arguments;
            //         changed_args[0] = noun;

            //         // TODO - find a more efficient way of not inferring certain already-inferred things
            //         auto new_expr = construct_expression(new_tupe, changed_args);
            //         if (inferred_predicates.count(new_expr) == 0) {
            //             auto was_inserted = inferred_predicates.insert(new_expr);
            //             if (was_inserted.second) {
            //                 expressions.push_back(new_expr);
            //             }
            //         }
            //     }
            // }
        }
    }
}

ResponseType Mind::ask(Expression expression)
{
    // simple yes/no as of now
    // auto asStatement = Predicate(queryPredicate.type_id, queryPredicate.arguments);

    auto is_given = given_expressions.count(expression) != 0;
    auto is_inferred = inferred_expressions.count(expression) != 0;

    if (is_given || is_inferred)
    {
        return ResponseType::YES;
    }

    return ResponseType::NO;
}

void Mind::infer(Expression expression)
{
    inferred_expressions.emplace(expression);
    expressions.push_back(pair<KnowledgeType, Expression> (KnowledgeType::INFERRED, expression));
}

void Mind::apply_inheritance_rule(Expression expression)
{
    int LOOP_LIMIT = 3;

    if (DEBUGGING)
        printf("applying inheritance rule\n");

    // vector<Expression> generated_expressions = vector<Expression>();
    // IS (3) -> CONTAINS -> IS
    auto connection_pairs = expression.get_connections(
        "IS", "object",
        "CONTAINS", "container");

    for (auto connection_pair : connection_pairs)
    {
        Predicate is_predicate = connection_pair.first;

        if (!equals(is_predicate.get_argument("object_count"), "3"))
        {
            // don't do this rule anymore
            return;
        }

        Predicate contains_predicate = connection_pair.second;

        // auto parent_child_pairs = vector<tuple<string, string>>();

        auto second_connection_pairs = expression.get_connections(
            "CONTAINS", "containee",
            "IS", "object");

        if (second_connection_pairs.size() == 0)
            return;
        
        for (auto second_connection_pair : second_connection_pairs)
        {
            Predicate other_is_predicate = second_connection_pair.first;

            string child = is_predicate.get_argument("noun_class");
            string parent = other_is_predicate.get_argument("noun_class");

            // add the map connections
            child_to_parent_map.emplace(child, parent);
            // child_to_parent.emplace(child, parent);
            // parent_child_pairs.push_back(make_tuple(parent, child));
        }
    }

    return;

    // for each new parent child pair, create the 
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


void Mind::apply_ability_rule(Expression expression)
{
    if (DEBUGGING)
        printf("applying ability rule\n");

    auto actor_ability_pairs = expression.get_connections(
        "IS", "object",
        "CAN_DO", "actor");

    for (auto actor_ability_pair : actor_ability_pairs)
    {
        Predicate actor_predicate = actor_ability_pair.first;
        Predicate ability_predicate = actor_ability_pair.second;

        string actor_arg = actor_predicate.get_argument("noun_class");
        string ability_arg = ability_predicate.get_argument("action_type");
        if (ability_map.count(actor_arg) == 0)
        {
            ability_map.emplace(
                actor_arg,
                vector<string>{ability_arg});
        }
        else {
            ability_map.at(actor_arg).push_back(ability_arg);
        }
    }
}

void Mind::update_conceptual_maps(Expression new_expression)
{
    printf("updating conceptual maps\n");
    apply_inheritance_rule(new_expression);
    apply_ability_rule(new_expression);

    // for (auto pred_of_type : expressions)
    // {
    //     if (pred_of_type.first == KnowledgeType::INFERRED)
    //         continue;
        
    //     auto pred = pred_of_type.second;

    //     // is a given predicate
    //     if (pred.type == PredicateType::IS_SUBSET_OF) {
    //         inheritance_map.emplace(pred.arguments[0], pred.arguments[1]);
    //     }

    //     for (string arg : pred.arguments)
    //     {
    //         entity_set.emplace(arg);
    //     }

    //     // populate the first_arg_to_predicate_map
    //     string first_arg = pred.arguments[0];

    //     if (first_arg_to_predicate_map.count(first_arg) != 0)
    //     {
    //         first_arg_to_predicate_map.at(first_arg).push_back(pred);
    //     } else {
    //         vector<Predicate> preds = {pred};
    //         first_arg_to_predicate_map.emplace(first_arg, preds);
    //     }
    // }
}


// void InheritanceGraph::add_node(InheritanceNode new_node)
// {
//     // just update the children of your children, don't care about parents pointers yet. no need.
//     stack<string> children_updating_stack;
//     if (new_node.children.size() != 0)
//     {
//         for (string child : new_node.children)
//         {
//             children_updating_stack.push(child);
//         }
//     }

//     // traverse to update parents and children
//     while (children_updating_stack.size() > 0)
//     {
//         string child_to_update = children_updating_stack.top();

//         if (nodes_by_name.count(child_to_update) == 0)
//            throw runtime_error(" ia m on huappy");

//         InheritanceNode node = nodes_by_name.at(child_to_update);

//         node.children = set_union(node.children, new_node.children);

//     }
// }
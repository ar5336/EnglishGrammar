#include "inference_rule.hpp"

// bool InferenceRule::do_conditions_apply(Expression basis)
// {
//     // given the conditions example of
//     // IF
//     //     IS object:a object_count:_ noun_class:c
//     //     IS object:b object_count:_ noun_class:d
//     //     CONTAINS container:a containee:b
//     //     CAN_DO action_type:e actor:a 
//     // THEN
//     //     CAN_DO action_type:e actor:b
//     //
//     //
//     return false;
// }

InferenceRule::InferenceRule(
    PredicateHandler* predicate_handler,
    const string &name,
    const vector<PredicateMatcher> &condition_template,
    const vector<PredicateCreator> &conclusion_template)
    :
        predicate_handler(predicate_handler),
        name(name),
        condition_template(condition_template),
        conclusion_template(conclusion_template)
{
    // construct the condition_as_expression expression for connection handling
    auto predicates = vector<Predicate>();

    for (auto condition_predicate: condition_template)
    {
        // auto arg_types = condition_predicate.parameter_matching_types;
        // auto predicate_template = condition_predicate.predicate_template;
        // int type_id = predicate_handler->string_to_type_id(predicate_template.predicate);
        if (condition_predicate.is_bound_predicates_matcher)
        {
            has_all_connected_predicate_matcher = true;
            continue;
        }

        string predicate_string = condition_predicate.stringify();
        auto predicate = predicate_handler->construct_predicate(condition_predicate.predicate_template.predicate, condition_predicate.param_values);

        predicates.push_back(predicate);
    }

    condition_as_expression = Expression(predicates);
}

// bool InferenceRule::try_make_conclusion(Expression potential_basis, Expression *conclusion)
// {
//     if (do_conditions_apply(potential_basis))
//     {
//         // apply the conclustion template to teh extracted basis
//         *conclusion = conclusions;
//         return true;
//     }
//     return false;
// }

InferencePredicateConnections InferenceHandler::identify_condition_connections(Expression condition_expression)
{
    // to do this, we need to implement a new method for Expression called get_occurrences_of_param
    //      which takes in
    //          predicate and argname and param
    //      puts out
    //          list of <predicates and argname>
    //  except for cases of _

    InferencePredicateConnections connections = InferencePredicateConnections();
    int condition_predicate_index = 0;
    for (int condition_predicate_index = 0; condition_predicate_index < condition_expression.predicates.size(); condition_predicate_index++)
    {
        auto condition_predicate = condition_expression.predicates[condition_predicate_index];

        string source_predicate_type_name = condition_predicate.predicate_template.predicate;
        for (int arg_i = 0; arg_i < condition_predicate.predicate_template.parameter_names.size(); arg_i++)
        {
            string arg_name = condition_predicate.predicate_template.parameter_names[arg_i];
            string param_value = condition_predicate.arguments[arg_i];

            if (param_value == "_")
                continue;

            auto occurrences_of_param = condition_expression.get_occurrences_of_param(param_value);

            for (auto occurrence : occurrences_of_param)
            {
                
                string occurrence_predicate_type_name = occurrence.predicate_type;
                string occurrence_param_name = occurrence.argument_name;

                if (DEBUGGING)
                    printf("occurrence: predicate type %s with argument %s\n", occurrence_predicate_type_name.c_str(), occurrence_param_name.c_str());

                if (equals(source_predicate_type_name, occurrence_predicate_type_name)
                    && equals(arg_name, occurrence_param_name)
                    && (condition_predicate_index == occurrence.predicate_index))
                    continue;
                    
                // add to existing connections
                connections.add_connection(
                    PredicateArgumentAddress(
                        condition_predicate_index,
                        source_predicate_type_name,
                        arg_name),
                    PredicateArgumentAddress(
                        occurrence.predicate_index,
                        occurrence_predicate_type_name,
                        occurrence_param_name));
            }

            if (DEBUGGING)
                printf("found %ld occurrences of parameter %s in predicate %s with value %s\n",
                    occurrences_of_param.size(),
                    arg_name.c_str(),
                    source_predicate_type_name.c_str(),
                    param_value.c_str());
            
            // identify if the connnections that are found are compatible 

            // if (existing_predicate_connections.count(predicate_type) == 0)
            // {
            //     existing_predicate_connections.emplace(predicate_type, map<string, pair<string, string>>());
            // }

            // if (existing_predicate_connections.at(predicate_type).count(arg_name) == 0)
            // {
            //     existing_predicate_connections.at(predicate_type).emplace(arg_name, make_pair(predicate_type, arg_name));
            // }
        }

        // if (DEBUGGING)
        // {
            printf("predicate '%s' has %ld parameters\n", source_predicate_type_name.c_str(), condition_predicate.predicate_template.parameter_names.size());
            for (int arg_i = 0; arg_i < condition_predicate.predicate_template.parameter_names.size(); arg_i++)
            {
                printf("    parameter %d: %s\n", arg_i, condition_predicate.predicate_template.parameter_names[arg_i].c_str());
            }
        // }
    }
    return connections;
}

InferenceHandler::InferenceHandler()
{}

InferenceHandler::InferenceHandler(vector<InferenceRule> inference_rules, PredicateHandler* predicate_handler)
    : predicate_handler(predicate_handler)
{
    for (auto inference_rule : inference_rules)
    {
        add_inference_rule(inference_rule);
    }
} 

void InferenceHandler::add_inference_rule(const InferenceRule &rule)
{
    if (inference_rules_by_name.count(rule.name) != 0)
        throw runtime_error("duplicate inference rule name: " + rule.name);

    inference_rules_by_name.emplace(rule.name, rule);
    auto condition_connections = identify_condition_connections(rule.condition_as_expression);
    inference_rule_predicate_connections.emplace(rule.name, condition_connections);
}

vector<pair<Expression, string>> InferenceHandler::apply_inference_rules(Expression knowledge_base)
{
    for (auto& [name, inference_rule] : inference_rules_by_name)
    {
        // check if this rule applies to the mass of knowledge
        if (inference_rule_predicate_connections.count(name) == 0)
            throw runtime_error("inference rule '" + name + "' has no identified predicate connections");

        // potential algorithms to implement to perform subgraph matching of the condition_as_expression in the knowledge base
        // Filtering-Ordering-Enumeration framework
        
        auto condition_connections = inference_rule_predicate_connections.at(name);

        printf("applying inference rule '%s' with %ld condition connections\n", name.c_str(), condition_connections.source_predicate_connection.size());

        auto candidate_kb_connections = vector<pair<PredicateArgumentAddress, PredicateArgumentAddress>>();

        for (auto& [source_predicate_address, target_predicate_address_set] : condition_connections.source_predicate_connection)
        {
            for (auto& target_predicate_address : target_predicate_address_set)
            {
                string source_predicate_name = source_predicate_address.predicate_type;
                string source_argument = source_predicate_address.argument_name;
                string target_predicate_name = target_predicate_address.predicate_type;
                string target_argument = target_predicate_address.argument_name;

                if (equals(source_predicate_name, target_predicate_name)
                    && equals(source_argument, target_argument)
                    && (source_predicate_address.predicate_index == target_predicate_address.predicate_index))
                    continue;


                // Apply the inference rule to the knowledge base

                auto base_connections = knowledge_base.get_connections(
                    source_predicate_name, source_argument,
                    target_predicate_name, target_argument);
                
                if (base_connections.size() == 0)
                    continue;

                printf("\tknowledge base has %ld connections between predicate '%s.%s' and '%s.%s'\n",
                    base_connections.size(),
                    source_predicate_name.c_str(), source_argument.c_str(),
                    target_predicate_name.c_str(), target_argument.c_str());

                for (auto& [source_predicate_address, target_predicate_address] : base_connections)
                {
                    // TODO - use the source and target predicates to construct the inferred expression based on the conclusion template of the inference rule
                    auto source_predicate = knowledge_base.predicates[source_predicate_address.predicate_index];
                    auto target_predicate = knowledge_base.predicates[target_predicate_address.predicate_index];
                    
                    string pred_1_str = predicate_handler->stringify_predicate(source_predicate);
                    string pred_2_str = predicate_handler->stringify_predicate(target_predicate);
                    printf("\t\tfound connection between predicate '%s' and '%s' by argument\n",
                        pred_1_str.c_str(),
                        pred_2_str.c_str());

                }

                // for now we just handle the first predicate pair identified
                auto connection = base_connections[0];
                auto source_predicate = connection.first;
                auto target_predicate = connection.second;

                // check if currently found predicate is compatible with the previous ones
                
                // inference_rule
                // need to use the information in the inheritance rule itself to determine the candidate connections
                
                // candidate_kb_connections.push_back(
                //     make_pair(
                //         make_pair(
                //             source_predicate,
                //             source_argument),
                //         make_pair(
                //             target_predicate,
                //             target_argument)));
                


                // connections must be satisfied.
            }
        }

        // for (auto condition_connection : inference_rule.condition_as_expression.get_connections())
    }
    return vector<pair<Expression, string>>();
}

bool operator<(const PredicateArgumentAddress& lhs, const PredicateArgumentAddress& rhs)
{
    return tie(lhs.predicate_type,
            lhs.argument_name,
            lhs.predicate_index,
            lhs.statement_index)
        < tie(rhs.predicate_type,
            rhs.argument_name,
            rhs.predicate_index,
            rhs.statement_index);
}
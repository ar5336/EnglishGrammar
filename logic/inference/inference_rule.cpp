#include "inference_rule.hpp"

bool InferenceRule::do_conditions_apply(Expression basis)
{
    // given the conditions example of
    // IF
    //     IS object:a object_count:_ noun_class:c
    //     IS object:b object_count:_ noun_class:d
    //     CONTAINS container:a containee:b
    //     CAN_DO action_type:e actor:a 
    // THEN
    //     CAN_DO action_type:e actor:b
    //
    //
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

void InferenceHandler::add_inference_rule(const InferenceRule &rule)
{
    inference_rules.push_back(rule);

    // argument value -> (predicate type, parameter name)
    // map<string, vector<pair<string, string>>> predicate_connections_for_rule;

    // // first, create a map of arguments to their predicate types and parameter names
    // for (PredicateMatcher condition_predicate : rule.condition_template)
    // {
    //     for (int i = 0; i < condition_predicate.predicate_template.parameter_names.size(); i++)
    //     {
    //         string param_name = condition_predicate.predicate_template.parameter_names[i];
    //         string predicate_type = condition_predicate.predicate_template.predicate;
    //         predicate_connections_for_rule[param_name].push_back(make_pair(predicate_type, param_name));
    //         // existing_predicate_connections[predicate_type][param_name] = make_pair(predicate_type, param_name);
    //     }
    // }

    // // traverse the predicates in the condition template and add the connections to the inference_rule_predicate_connections map
    // for (auto condition_predicate : rule.condition_template)
    // {
    //     inference_rule_predicate_connections
    //         [rule.name]
    //         [condition_predicate.predicate_template.predicate]
    //         [condition_predicate.predicate_template.predicate]
    //          = make_pair(condition_predicate.get_source_argument(), condition_predicate.get_target_argument());
    // }
}

vector<Expression> InferenceHandler::apply_inference_rules(Expression knowledge_base)
{
    return vector<Expression>();
}

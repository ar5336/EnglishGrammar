#include "inference_rule.hpp"

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
    vector<Predicate> predicates;
    set<string> variables;

    for (const auto& condition_predicate : condition_template)
    {
        if (condition_predicate.is_bound_predicates_matcher)
        {
            has_all_connected_predicate_matcher = true;
            continue;
        }

        const auto& predicate_template = condition_predicate.predicate_template;
        vector<string> arguments(predicate_template.parameter_names.size(), "_");
        set<string> parameter_names;
        for (int i = 0; i < condition_predicate.param_names.size(); i++)
        {
            const auto& parameter_name = condition_predicate.param_names[i];
            if (predicate_template.parameter_index_map.count(parameter_name) == 0)
                throw runtime_error("unknown condition parameter '" + parameter_name + "' in inference rule '" + name + "'");
            if (!parameter_names.insert(parameter_name).second)
                throw runtime_error("duplicate condition parameter '" + parameter_name + "' in inference rule '" + name + "'");

            auto matching_type = condition_predicate.parameter_matching_types[i];
            if (matching_type == WORD_FRAME)
            {
                const auto& variable = condition_predicate.param_values[i];
                variables.insert(variable);
                arguments[predicate_template.parameter_index_map.at(parameter_name)] = variable;
            }
            else if (matching_type != STRING && matching_type != DROP)
            {
                throw runtime_error("unsupported condition parameter type in inference rule '" + name + "'");
            }
        }

        // Only variables form graph edges; literals and ignored arguments are checked during matching.
        predicates.push_back(predicate_handler->construct_predicate(predicate_template.predicate, arguments));
    }

    condition_as_expression = Expression(predicates);

    for (const auto& creator : conclusion_template)
    {
        if (creator.is_bound_predicate_creator)
        {
            has_all_connected_predicate_creator = true;
            continue;
        }

        for (auto creation_type : creator.parameter_creation_types)
        {
            if (creation_type != WORD_FRAME && creation_type != STRING)
                throw runtime_error("inference rule '" + name + "' conclusions require bound variables or quoted strings");
        }
        for (const auto& variable : creator.word_frame_accessors)
        {
            if (variables.count(variable) == 0)
                throw runtime_error("unbound conclusion variable '" + variable + "' in inference rule '" + name + "'");
        }
    }
}

bool InferenceRule::uses_all_connected_predicates() const
{
    return has_all_connected_predicate_matcher || has_all_connected_predicate_creator;
}

InferencePredicateConnections InferenceHandler::identify_condition_connections(Expression condition_expression)
{
    InferencePredicateConnections connections;
    for (int condition_predicate_index = 0; condition_predicate_index < condition_expression.predicates.size(); condition_predicate_index++)
    {
        const auto& condition_predicate = condition_expression.predicates[condition_predicate_index];
        for (int arg_i = 0; arg_i < condition_predicate.arguments.size(); arg_i++)
        {
            const auto& param_value = condition_predicate.arguments[arg_i];
            if (param_value == "_")
                continue;

            PredicateArgumentAddress source(
                condition_predicate_index,
                condition_predicate.predicate_template.predicate,
                condition_predicate.predicate_template.parameter_names[arg_i]);
            for (const auto& occurrence : condition_expression.get_occurrences_of_param(param_value))
            {
                if (!(source == occurrence))
                    connections.add_connection(source, occurrence);
            }
        }
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

    if (rule.uses_all_connected_predicates())
        fprintf(stderr, "Inference rule '%s' is deferred: all_connected matching and copying are not implemented.\n", rule.name.c_str());
}

namespace
{
class InferenceMatcher
{
    const InferenceRule& rule;
    Expression& knowledge_base;
    PredicateHandler* predicate_handler;
    const set<Predicate>& known_predicates;
    vector<vector<int>> candidates;
    vector<set<int>> neighbors;

    bool match_predicate(const PredicateMatcher& matcher, Predicate& predicate, map<string, string>& bindings)
    {
        if (matcher.predicate_template.predicate != predicate.predicate_template.predicate)
            return false;

        for (int i = 0; i < matcher.param_names.size(); i++)
        {
            if (matcher.parameter_matching_types[i] == DROP)
                continue;

            string value;
            if (!predicate.try_get_argument(matcher.param_names[i], value))
                return false;

            switch (matcher.parameter_matching_types[i])
            {
            case STRING:
                if (matcher.param_values[i] != value)
                    return false;
                break;
            case WORD_FRAME:
                {
                    auto binding = bindings.emplace(matcher.param_values[i], value);
                    if (!binding.second && binding.first->second != value)
                        return false;
                    break;
                }
            default:
                throw runtime_error("unsupported condition parameter type in inference rule '" + rule.name + "'");
            }
        }
        return true;
    }

    bool make_conclusion(const map<string, string>& bindings, Expression& conclusion)
    {
        vector<Predicate> predicates;
        set<Predicate> included;
        bool has_new_information = false;

        for (const auto& creator : rule.conclusion_template)
        {
            vector<string> arguments;
            int variable_index = 0;
            int string_index = 0;
            for (auto creation_type : creator.parameter_creation_types)
            {
                if (creation_type == WORD_FRAME)
                    arguments.push_back(bindings.at(creator.word_frame_accessors.at(variable_index++)));
                else if (creation_type == STRING)
                    arguments.push_back(creator.param_strings.at(string_index++));
                else
                    throw runtime_error("unsupported conclusion parameter type in inference rule '" + rule.name + "'");
            }

            auto predicate = predicate_handler->construct_predicate(creator.predicate.predicate, arguments);
            if (included.insert(predicate).second)
            {
                predicates.push_back(predicate);
                if (known_predicates.count(predicate) == 0)
                    has_new_information = true;
            }
        }

        if (!has_new_information)
            return false;

        printf("inference rule '%s' produced:\n%s", rule.name.c_str(), predicate_handler->stringify_expression(Expression(predicates)).c_str());

        conclusion = Expression(predicates);
        return true;
    }

    bool broaden(vector<int>& matched_predicates, const map<string, string>& bindings, Expression& conclusion)
    {
        int next_condition = -1;
        bool next_is_connected = false;
        for (int i = 0; i < candidates.size(); i++)
        {
            if (matched_predicates[i] != -1)
                continue;

            bool is_connected = false;
            for (int neighbor : neighbors[i])
            {
                if (matched_predicates[neighbor] != -1)
                    is_connected = true;
            }

            if (next_condition == -1
                || (is_connected && !next_is_connected)
                || (is_connected == next_is_connected && candidates[i].size() < candidates[next_condition].size()))
            {
                next_condition = i;
                next_is_connected = is_connected;
            }
        }

        if (next_condition == -1)
            return make_conclusion(bindings, conclusion);

        for (int predicate_index : candidates[next_condition])
        {
            auto next_bindings = bindings;
            if (!match_predicate(rule.condition_template[next_condition], knowledge_base.predicates[predicate_index], next_bindings))
                continue;

            matched_predicates[next_condition] = predicate_index;
            if (broaden(matched_predicates, next_bindings, conclusion))
                return true;
            matched_predicates[next_condition] = -1;
        }
        return false;
    }

public:
    InferenceMatcher(
        const InferenceRule& rule,
        const InferencePredicateConnections& connections,
        Expression& knowledge_base,
        PredicateHandler* predicate_handler,
        const set<Predicate>& known_predicates)
        : rule(rule),
          knowledge_base(knowledge_base),
          predicate_handler(predicate_handler),
          known_predicates(known_predicates),
          candidates(rule.condition_template.size()),
          neighbors(rule.condition_template.size())
    {
        for (int i = 0; i < rule.condition_template.size(); i++)
        {
            for (int predicate_index = 0; predicate_index < knowledge_base.predicates.size(); predicate_index++)
            {
                map<string, string> bindings;
                if (match_predicate(rule.condition_template[i], knowledge_base.predicates[predicate_index], bindings))
                    candidates[i].push_back(predicate_index);
            }
        }

        for (const auto& [source, targets] : connections.source_predicate_connection)
        {
            for (const auto& target : targets)
                neighbors[source.predicate_index].insert(target.predicate_index);
        }
    }

    bool try_infer(Expression& conclusion)
    {
        for (const auto& predicate_candidates : candidates)
        {
            if (predicate_candidates.empty())
                return false;
        }

        vector<int> matched_predicates(candidates.size(), -1);
        return broaden(matched_predicates, map<string, string>(), conclusion);
    }
};
}

vector<pair<Expression, string>> InferenceHandler::apply_inference_rules(Expression knowledge_base)
{
    vector<pair<Expression, string>> results;
    set<Predicate> known_predicates(knowledge_base.predicates.begin(), knowledge_base.predicates.end());
    
    for (const auto& [name, inference_rule] : inference_rules_by_name)
    {
        if (inference_rule.uses_all_connected_predicates())
            continue;
        if (inference_rule_predicate_connections.count(name) == 0)
            throw runtime_error("inference rule '" + name + "' has no identified predicate connections");

        InferenceMatcher matcher(
            inference_rule,
            inference_rule_predicate_connections.at(name),
            knowledge_base,
            predicate_handler,
            known_predicates);
        Expression conclusion;
        if (matcher.try_infer(conclusion))
        {
            if (DEBUGGING)
                printf("inference rule '%s' produced:\n%s", name.c_str(), predicate_handler->stringify_expression(conclusion).c_str());
            results.emplace_back(conclusion, name);
            known_predicates.insert(conclusion.predicates.begin(), conclusion.predicates.end());
        }
    }
    return results;
}
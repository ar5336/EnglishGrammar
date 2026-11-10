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
    const PredicateMatcher* connected_matcher = nullptr;

    for (const auto& condition_predicate : condition_template)
    {
        if (condition_predicate.is_bound_predicates_matcher)
        {
            if (connected_matcher != nullptr)
                throw runtime_error("inference rule '" + name + "' has more than one all_connected capture");
            connected_matcher = &condition_predicate;
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

    if (connected_matcher != nullptr && variables.count(connected_matcher->param_values.at(0)) == 0)
        throw runtime_error("unbound all_connected source variable '" + connected_matcher->param_values.at(0) + "' in inference rule '" + name + "'");

    for (const auto& creator : conclusion_template)
    {
        if (creator.is_bound_predicate_creator)
        {
            if (connected_matcher == nullptr)
                throw runtime_error("all_connected conclusion without a capture in inference rule '" + name + "'");
            if (variables.count(creator.param_strings.at(0)) == 0)
                throw runtime_error("unbound all_connected target variable '" + creator.param_strings.at(0) + "' in inference rule '" + name + "'");
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
}

namespace
{
class InferenceContext
{
    const KnowledgeBase& knowledge_base;

public:
    Expression knowledge_expression;
    set<Predicate> known_predicates;

    explicit InferenceContext(const KnowledgeBase& knowledge_base)
        : knowledge_base(knowledge_base),
          knowledge_expression(knowledge_base.get_combined_facts()),
          known_predicates(knowledge_expression.predicates.begin(), knowledge_expression.predicates.end())
    {}

    bool can_expand_predicate(int predicate_index, const string& rule_name) const
    {
        int fact_index = knowledge_base.get_fact_index(predicate_index);
        const auto& source = knowledge_base.get_facts().at(fact_index).get_source();
        return source.type != KnowledgeSourceType::INFERRED || source.rule_name != rule_name;
    }
};

class InferenceMatcher
{
    const InferenceRule& rule;
    InferenceContext& context;
    PredicateHandler* predicate_handler;
    vector<const PredicateMatcher*> conditions;
    const PredicateMatcher* connected_matcher = nullptr;
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

    vector<int> capture_connected_predicates(const string& root, const vector<int>& matched_predicates)
    {
        set<Predicate> excluded;
        for (int predicate_index : matched_predicates)
            excluded.insert(context.knowledge_expression.predicates[predicate_index]);

        vector<int> captured;
        set<Predicate> visited_predicates;
        set<string> visited_variables;
        vector<string> frontier{root};
        while (!frontier.empty())
        {
            string variable = frontier.back();
            frontier.pop_back();
            if (!visited_variables.insert(variable).second)
                continue;

            for (const auto& occurrence : context.knowledge_expression.get_occurrences_of_param(variable))
            {
                // Previous outputs of this rule are traversal barriers, not just omitted output.
                if (!context.can_expand_predicate(occurrence.predicate_index, rule.name))
                    continue;

                auto& predicate = context.knowledge_expression.predicates[occurrence.predicate_index];
                if (!predicate.predicate_template.is_param_schematic(occurrence.argument_name)
                    || excluded.count(predicate) != 0
                    || !visited_predicates.insert(predicate).second)
                    continue;

                captured.push_back(occurrence.predicate_index);
                for (int i = 0; i < predicate.arguments.size(); i++)
                {
                    if (predicate.predicate_template.is_param_schematic(predicate.predicate_template.parameter_names[i]))
                        frontier.push_back(predicate.arguments[i]);
                }
            }
        }
        return captured;
    }

    bool make_conclusion(
        const vector<int>& matched_predicates,
        const map<string, string>& bindings,
        Expression& conclusion,
        vector<int>& source_predicate_indices)
    {
        vector<Predicate> predicates;
        set<Predicate> included;
        bool has_new_information = false;
        // lambda function
        auto include_predicate = [&](const Predicate& predicate)
        {
            if (included.insert(predicate).second)
            {
                predicates.push_back(predicate);
                if (context.known_predicates.count(predicate) == 0)
                    has_new_information = true;
            }
        };

        vector<int> connected_predicates;
        string source;
        if (connected_matcher != nullptr)
        {
            source = bindings.at(connected_matcher->param_values.at(0));
            connected_predicates = capture_connected_predicates(source, matched_predicates);
        }

        for (const auto& creator : rule.conclusion_template)
        {
            // case that covers the * (all_connected) case
            if (creator.is_bound_predicate_creator)
            {
                const auto& target = bindings.at(creator.param_strings.at(0));
                for (int predicate_index : connected_predicates)
                {
                    auto predicate = context.knowledge_expression.predicates[predicate_index];
                    for (int i = 0; i < predicate.arguments.size(); i++)
                    {
                        const auto& parameter_name = predicate.predicate_template.parameter_names[i];
                        if (predicate.arguments[i] == source && predicate.predicate_template.is_param_schematic(parameter_name))
                            predicate = predicate.with_modified_argument(parameter_name, target);
                    }
                    include_predicate(predicate);
                }
                continue;
            }

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
            include_predicate(predicate);
        }

        if (!has_new_information)
            return false;

        set<int> used_predicates(matched_predicates.begin(), matched_predicates.end());
        used_predicates.insert(connected_predicates.begin(), connected_predicates.end());
        source_predicate_indices.assign(used_predicates.begin(), used_predicates.end());
        conclusion = Expression(predicates);

        printf("inference rule '%s' produced:\n%s",
            rule.name.c_str(),
            predicate_handler->stringify_expression(conclusion, 1).c_str());
        
        return true;
    }

    bool broaden(
        vector<int>& matched_predicates,
        const map<string, string>& bindings,
        Expression& conclusion,
        vector<int>& source_predicate_indices)
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
            return make_conclusion(matched_predicates, bindings, conclusion, source_predicate_indices);

        for (int predicate_index : candidates[next_condition])
        {
            auto next_bindings = bindings;
            if (!match_predicate(*conditions[next_condition], context.knowledge_expression.predicates[predicate_index], next_bindings))
                continue;

            matched_predicates[next_condition] = predicate_index;
            if (broaden(matched_predicates, next_bindings, conclusion, source_predicate_indices))
                return true;
            matched_predicates[next_condition] = -1;
        }
        return false;
    }

public:
    InferenceMatcher(
        const InferenceRule& rule,
        const InferencePredicateConnections& connections,
        InferenceContext& context,
        PredicateHandler* predicate_handler)
        : rule(rule),
          context(context),
          predicate_handler(predicate_handler)
    {
        for (const auto& condition : rule.condition_template)
        {
            if (condition.is_bound_predicates_matcher)
                connected_matcher = &condition;
            else
                conditions.push_back(&condition);
        }

        candidates.resize(conditions.size());
        neighbors.resize(conditions.size());
        for (int i = 0; i < conditions.size(); i++)
        {
            for (int predicate_index = 0; predicate_index < context.knowledge_expression.predicates.size(); predicate_index++)
            {
                map<string, string> bindings;
                if (match_predicate(*conditions[i], context.knowledge_expression.predicates[predicate_index], bindings))
                    candidates[i].push_back(predicate_index);
            }
        }

        for (const auto& [source, targets] : connections.source_predicate_connection)
        {
            for (const auto& target : targets)
                neighbors[source.predicate_index].insert(target.predicate_index);
        }
    }

    bool try_infer(Expression& conclusion, vector<int>& source_predicate_indices)
    {
        for (const auto& predicate_candidates : candidates)
        {
            if (predicate_candidates.empty())
                return false;
        }

        vector<int> matched_predicates(candidates.size(), -1);
        return broaden(matched_predicates, map<string, string>(), conclusion, source_predicate_indices);
    }
};
}

vector<Knowledge> InferenceHandler::apply_inference_rules(const KnowledgeBase& knowledge_base)
{
    vector<Knowledge> results;
    InferenceContext context(knowledge_base);
    
    for (const auto& [name, inference_rule] : inference_rules_by_name)
    {
        if (inference_rule_predicate_connections.count(name) == 0)
            throw runtime_error("inference rule '" + name + "' has no identified predicate connections");

        InferenceMatcher matcher(
            inference_rule,
            inference_rule_predicate_connections.at(name),
            context,
            predicate_handler);
        Expression conclusion;
        vector<int> source_predicate_indices;
        if (matcher.try_infer(conclusion, source_predicate_indices))
        {
            if (DEBUGGING)
                printf("inference rule '%s' produced:\n%s",
                    name.c_str(),
                    predicate_handler->stringify_expression(conclusion, 1).c_str());

            set<int> source_indices;
            for (int predicate_index : source_predicate_indices)
                source_indices.insert(knowledge_base.get_fact_index(predicate_index));
            context.known_predicates.insert(conclusion.predicates.begin(), conclusion.predicates.end());
            results.emplace_back(std::move(conclusion), KnowledgeSource{
                KnowledgeSourceType::INFERRED,
                vector<int>(source_indices.begin(), source_indices.end()),
                name});
        }
    }
    return results;
}

vector<pair<Expression, string>> InferenceHandler::apply_inference_rules(Expression expression)
{
    KnowledgeBase knowledge_base(nullptr);
    knowledge_base.tell_fact(std::move(expression));

    vector<pair<Expression, string>> results;
    for (const auto& knowledge : apply_inference_rules(knowledge_base))
        results.emplace_back(knowledge.get_expression(), knowledge.get_source().rule_name);
    return results;
}
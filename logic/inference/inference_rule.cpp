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
    for (const auto& condition : rule.condition_template)
    {
        if (condition.is_bound_predicates_matcher)
            wildcard_dependents.insert(rule.name);
        else
            predicate_dependents[condition.predicate_template.predicate].insert(rule.name);
    }
}

namespace
{
class InferenceContext
{
    const KnowledgeBase& knowledge_base;

public:
    Expression knowledge_expression;

    explicit InferenceContext(const KnowledgeBase& knowledge_base)
        : knowledge_base(knowledge_base),
          knowledge_expression(knowledge_base.get_combined_facts())
    {}

    int get_expansion_source(int predicate_index, const string& rule_name) const
    {
        for (int fact_index : knowledge_base.get_fact_indices(predicate_index))
        {
            const auto& source = knowledge_base.get_facts().at(fact_index).get_source();
            if (source.type != KnowledgeSourceType::INFERRED || source.rule_name != rule_name)
                return fact_index;
        }
        return -1;
    }

    KnowledgeSource make_source(
        const string& rule_name,
        const vector<int>& matched_predicates,
        const vector<int>& connected_predicates) const
    {
        set<int> source_indices;
        auto include_support = [&](int predicate_index, int fact_index)
        {
            auto sources = knowledge_base.get_support_sources(predicate_index, fact_index);
            source_indices.insert(sources.begin(), sources.end());
        };
        for (int predicate_index : matched_predicates)
            include_support(predicate_index, knowledge_base.get_fact_index(predicate_index));
        for (int predicate_index : connected_predicates)
        {
            int source_index = get_expansion_source(predicate_index, rule_name);
            if (source_index == -1)
                throw logic_error("wildcard capture has no eligible source in inference rule '" + rule_name + "'");
            include_support(predicate_index, source_index);
        }
        return KnowledgeSource{
            KnowledgeSourceType::INFERRED,
            vector<int>(source_indices.begin(), source_indices.end()),
            rule_name,
            {}};
    }

    bool has_derivation(const Expression& expression, const KnowledgeSource& source) const
    {
        return knowledge_base.has_derivation(expression, source);
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
                // A previous output needs independent support before this rule can expand it.
                if (context.get_expansion_source(occurrence.predicate_index, rule.name) == -1)
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
        KnowledgeSource& provenance)
    {
        vector<Predicate> predicates;
        set<Predicate> included;
        // lambda function
        auto include_predicate = [&](const Predicate& predicate)
        {
            if (included.insert(predicate).second)
                predicates.push_back(predicate);
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

        set<int> used_predicates(matched_predicates.begin(), matched_predicates.end());
        used_predicates.insert(connected_predicates.begin(), connected_predicates.end());

        // Merely restating the selected premises is not independent evidence.
        set<Predicate> premises;
        for (int predicate_index : used_predicates)
            premises.insert(context.knowledge_expression.predicates[predicate_index]);
        bool restates_premises = true;
        for (const auto& predicate : predicates)
        {
            if (premises.count(predicate) == 0)
                restates_premises = false;
        }
        if (restates_premises)
            return false;

        conclusion = Expression(predicates);
        provenance = context.make_source(rule.name, matched_predicates, connected_predicates);
        for (int predicate_index : used_predicates)
            provenance.premise_predicates.push_back(context.knowledge_expression.predicates[predicate_index]);
        return !context.has_derivation(conclusion, provenance);
    }

    bool broaden(
        vector<int>& matched_predicates,
        const map<string, string>& bindings,
        Expression& conclusion,
        KnowledgeSource& provenance)
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
            return make_conclusion(matched_predicates, bindings, conclusion, provenance);

        for (int predicate_index : candidates[next_condition])
        {
            auto next_bindings = bindings;
            if (!match_predicate(*conditions[next_condition], context.knowledge_expression.predicates[predicate_index], next_bindings))
                continue;

            matched_predicates[next_condition] = predicate_index;
            if (broaden(matched_predicates, next_bindings, conclusion, provenance))
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

    bool try_infer(Expression& conclusion, KnowledgeSource& provenance)
    {
        for (const auto& predicate_candidates : candidates)
        {
            if (predicate_candidates.empty())
                return false;
        }

        vector<int> matched_predicates(candidates.size(), -1);
        return broaden(matched_predicates, map<string, string>(), conclusion, provenance);
    }
};
}

void InferenceHandler::schedule_rule(KnowledgeBase& knowledge_base, const string& name) const
{
    if (knowledge_base.scheduled_rules.insert(name).second)
        knowledge_base.inference_agenda.push_back(name);
}

void InferenceHandler::enqueue_inference_rules(KnowledgeBase& knowledge_base) const
{
    for (const auto& [name, inference_rule] : inference_rules_by_name)
    {
        if (inference_rule_predicate_connections.count(name) == 0)
            throw runtime_error("inference rule '" + name + "' has no identified predicate connections");
        schedule_rule(knowledge_base, name);
    }
}

bool InferenceHandler::advance_inference(KnowledgeBase& knowledge_base) const
{
    if (knowledge_base.inference_agenda.empty())
        return false;

    string name = knowledge_base.inference_agenda.front();
    knowledge_base.inference_agenda.pop_front();
    knowledge_base.scheduled_rules.erase(name);
    const auto& inference_rule = inference_rules_by_name.at(name);
    InferenceContext context(knowledge_base);
    InferenceMatcher matcher(
        inference_rule,
        inference_rule_predicate_connections.at(name),
        context,
        predicate_handler);
    Expression conclusion;
    KnowledgeSource source;
    if (!matcher.try_infer(conclusion, source))
        return false;

    auto changed_types = knowledge_base.add_knowledge(Knowledge(std::move(conclusion), std::move(source)));
    if (DEBUGGING)
    {
        int fact_index = knowledge_base.get_facts().size() - 1;
        int conclusion_index = knowledge_base.get_conclusion_index(fact_index);
        const auto& family = knowledge_base.get_conclusions().at(conclusion_index);
        if (family.phases.back().fact_index == fact_index)
        {
            printf("inference rule '%s' %s fact C%d, phase %zu [K%d]:\n",
                name.c_str(), family.phases.size() == 1 ? "constructed" : "expanded",
                conclusion_index, family.phases.size(), fact_index);
            for (const auto& predicate : knowledge_base.canonicalize(family.phases.back().additions).predicates)
                printf("\t[%zu] %s\n", family.phases.size(), predicate_handler->stringify_predicate(predicate).c_str());
        }
        else
        {
            printf("inference rule '%s' added evidence to fact C%d [K%d] from",
                name.c_str(), conclusion_index, fact_index);
            for (int source_index : knowledge_base.get_facts().back().get_source().source_indices)
                printf(" #%d", source_index);
            printf("\n");
        }
    }

    // Exhaust remaining matches without letting one rule monopolize the agenda.
    schedule_rule(knowledge_base, name);
    for (const auto& type : changed_types)
    {
        auto dependents = predicate_dependents.find(type);
        if (dependents != predicate_dependents.end())
        {
            for (const auto& dependent : dependents->second)
                schedule_rule(knowledge_base, dependent);
        }
    }
    if (!changed_types.empty())
    {
        for (const auto& dependent : wildcard_dependents)
            schedule_rule(knowledge_base, dependent);
    }
    return true;
}

vector<Knowledge> InferenceHandler::apply_inference_rules(const KnowledgeBase& knowledge_base)
{
    KnowledgeBase working_knowledge = knowledge_base;
    working_knowledge.inference_agenda.clear();
    working_knowledge.scheduled_rules.clear();
    enqueue_inference_rules(working_knowledge);
    vector<Knowledge> results;
    while (working_knowledge.has_pending_inferences())
    {
        if (advance_inference(working_knowledge))
            results.push_back(working_knowledge.get_facts().back());
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
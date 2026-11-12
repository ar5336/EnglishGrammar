#include "knowledge_base.hpp"
#include "inference/inference_rule.hpp"

KnowledgeBase::KnowledgeBase(InferenceHandler *inference_handler)
    : inference_handler(inference_handler)
{
}

set<string> KnowledgeBase::add_knowledge(Knowledge knowledge)
{
    int fact_index = facts.size();
    int conclusion_index;
    map<string, string> reference_bindings;
    const auto& expression = knowledge.get_expression();
    if (!try_get_conclusion(expression, conclusion_index, reference_bindings))
    {
        conclusion_index = conclusions.size();
        conclusions.push_back(KnowledgeConclusion{expression, {}});
        for (const auto& predicate : expression.predicates)
        {
            for (size_t i = 0; i < predicate.arguments.size(); i++)
            {
                if (predicate.predicate_template.are_params_schematic.at(i))
                    reference_bindings.emplace(predicate.arguments[i], predicate.arguments[i]);
            }
        }
    }
    conclusions[conclusion_index].evidence.push_back(KnowledgeEvidence{fact_index, std::move(reference_bindings)});

    set<string> changed_predicate_types;
    set<Predicate> processed;
    const auto& source = knowledge.get_source();
    auto combined_predicates = combined_facts.predicates;
    for (const auto& predicate : knowledge.get_expression().predicates)
    {
        if (!processed.insert(predicate).second)
            continue;

        auto& sources = predicate_sources[predicate];
        bool has_same_producer = false;
        for (int source_index : sources)
        {
            const auto& existing_source = facts.at(source_index).get_source();
            if (existing_source.type == source.type && existing_source.rule_name == source.rule_name)
            {
                has_same_producer = true;
                break;
            }
        }
        // A new producer may make an existing predicate eligible for wildcard expansion.
        if (!has_same_producer)
            changed_predicate_types.insert(predicate.predicate_template.predicate);
        if (sources.empty())
        {
            combined_predicates.push_back(predicate);
            expression_index_list_by_combined_facts_index.push_back(fact_index);
        }
        if (sources.empty() || sources.back() != fact_index)
            sources.push_back(fact_index);
    }

    facts.push_back(std::move(knowledge));
    if (combined_predicates.size() != combined_facts.predicates.size())
        combined_facts = Expression(std::move(combined_predicates));
    return changed_predicate_types;
}

void KnowledgeBase::tell_fact(Expression fact)
{
    enqueue_fact(std::move(fact));
    while (has_pending_inferences())
        advance_inference();
}

void KnowledgeBase::enqueue_fact(Expression fact)
{
    add_knowledge(Knowledge(std::move(fact), KnowledgeSource{KnowledgeSourceType::GIVEN, {}, "", {}}));
    if (inference_handler != nullptr)
        inference_handler->enqueue_inference_rules(*this);
}

bool KnowledgeBase::has_pending_inferences() const
{
    return !inference_agenda.empty();
}

bool KnowledgeBase::advance_inference()
{
    return inference_handler != nullptr && inference_handler->advance_inference(*this);
}

const vector<Knowledge>& KnowledgeBase::get_facts() const
{
    return facts;
}

const vector<KnowledgeConclusion>& KnowledgeBase::get_conclusions() const
{
    return conclusions;
}

bool KnowledgeBase::try_get_conclusion(
    const Expression& expression,
    int& conclusion_index,
    map<string, string>& reference_bindings) const
{
    for (size_t i = 0; i < conclusions.size(); i++)
    {
        if (conclusions[i].expression.try_match_topology(expression, reference_bindings))
        {
            conclusion_index = i;
            return true;
        }
    }
    conclusion_index = -1;
    reference_bindings.clear();
    return false;
}

bool KnowledgeBase::has_derivation(const Expression& expression, const KnowledgeSource& source) const
{
    set<Predicate> predicates(expression.predicates.begin(), expression.predicates.end());
    for (const auto& fact : facts)
    {
        const auto& existing_source = fact.get_source();
        if (existing_source.type != source.type
            || existing_source.rule_name != source.rule_name
            || existing_source.source_indices != source.source_indices
            || existing_source.predicate_indices != source.predicate_indices)
            continue;

        const auto& existing = fact.get_expression().predicates;
        if (predicates == set<Predicate>(existing.begin(), existing.end()))
            return true;
    }
    return false;
}

const Expression& KnowledgeBase::get_combined_facts() const
{
    return combined_facts;
}

int KnowledgeBase::get_fact_index(int combined_predicate_index) const
{
    return expression_index_list_by_combined_facts_index.at(combined_predicate_index);
}

const vector<int>& KnowledgeBase::get_fact_indices(int combined_predicate_index) const
{
    return predicate_sources.at(combined_facts.predicates.at(combined_predicate_index));
}

vector<string> KnowledgeBase::query(Expression query_expression)
{
    return vector<string>();
}
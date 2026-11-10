#include "knowledge_base.hpp"
#include "inference/inference_rule.hpp"

KnowledgeBase::KnowledgeBase(InferenceHandler *inference_handler)
    : inference_handler(inference_handler)
{
}

void KnowledgeBase::add_knowledge(Knowledge knowledge)
{
    int fact_index = facts.size();
    auto combined_predicates = combined_facts.predicates;
    for (const auto& predicate : knowledge.get_expression().predicates)
    {
        // Repeated context keeps its original supporting entry.
        if (known_predicates.insert(predicate).second)
        {
            combined_predicates.push_back(predicate);
            expression_index_list_by_combined_facts_index.push_back(fact_index);
        }
    }

    facts.push_back(std::move(knowledge));
    if (combined_predicates.size() != combined_facts.predicates.size())
        combined_facts = Expression(std::move(combined_predicates));
}

void KnowledgeBase::tell_fact(Expression fact)
{
    add_knowledge(Knowledge(std::move(fact), KnowledgeSource{KnowledgeSourceType::GIVEN, {}, ""}));
    if (inference_handler != nullptr)
    {
        auto inferred = inference_handler->apply_inference_rules(*this);
        for (auto& knowledge : inferred)
            add_knowledge(std::move(knowledge));
    }
}

const vector<Knowledge>& KnowledgeBase::get_facts() const
{
    return facts;
}

const Expression& KnowledgeBase::get_combined_facts() const
{
    return combined_facts;
}

int KnowledgeBase::get_fact_index(int combined_predicate_index) const
{
    return expression_index_list_by_combined_facts_index.at(combined_predicate_index);
}

vector<string> KnowledgeBase::query(Expression query_expression)
{
    return vector<string>();
}
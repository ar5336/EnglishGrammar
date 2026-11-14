#include "knowledge_base.hpp"
#include "inference/inference_rule.hpp"
#include <algorithm>

namespace
{
set<string> references(const Expression& expression)
{
    set<string> result;
    for (const auto& predicate : expression.predicates)
        for (size_t i = 0; i < predicate.arguments.size(); i++)
            if (predicate.predicate_template.are_params_schematic.at(i))
                result.insert(predicate.arguments[i]);
    return result;
}

map<string, string> shared_bindings(const Expression& left, const Expression& right)
{
    auto right_references = references(right);
    map<string, string> bindings;
    for (const auto& reference : references(left))
        if (right_references.count(reference))
            bindings.emplace(reference, reference);
    return bindings;
}

set<Predicate> predicate_set(const Expression& expression)
{
    return {expression.predicates.begin(), expression.predicates.end()};
}
}

KnowledgeBase::KnowledgeBase(InferenceHandler *inference_handler, EvidenceMode evidence_mode)
    : inference_handler(inference_handler), evidence_mode(evidence_mode)
{
}

set<string> KnowledgeBase::add_knowledge(Knowledge knowledge)
{
    int fact_index = facts.size();
    int conclusion_index = -1;
    map<string, string> bindings;
    auto expression = canonicalize(knowledge.get_expression());
    const auto& source = knowledge.get_source();
    bool first_proof = evidence_mode == EvidenceMode::FIRST_PROOF
        && source.type == KnowledgeSourceType::INFERRED;
    bool recorded = first_proof && recorded_expressions.count(predicate_set(expression));
    bool matched = !recorded && find_conclusion(expression, source, conclusion_index, bindings);
    if (first_proof && (recorded || (matched && !changes_conclusion(expression, conclusion_index, bindings))))
    {
        set<string> changed_types;
        for (const auto& predicate : expression.predicates)
            if (predicate_producers[predicate].emplace(source.type, source.rule_name).second)
                changed_types.insert(predicate.predicate_template.predicate);
        recorded_expressions.insert(predicate_set(expression));
        return changed_types;
    }

    auto old_producers = predicate_producers;
    for (const auto& reference : references(knowledge.get_expression()))
        reference_order.emplace(reference, reference_order.size());
    facts.push_back(std::move(knowledge));
    size_t previous_links = reference_links.size();
    if (matched)
    {
        map<string, string> inverse_bindings;
        for (const auto& [left, right] : bindings)
            inverse_bindings.emplace(right, left);
        map<Predicate, int> origins;
        for (const auto& phase : conclusions[conclusion_index].phases)
            for (const auto& predicate : canonicalize(phase.additions).predicates)
                origins.emplace(predicate, phase.fact_index);
        set<int> witnesses{fact_index};
        for (auto predicate : expression.predicates)
        {
            for (size_t i = 0; i < predicate.arguments.size(); i++)
            {
                if (!predicate.predicate_template.are_params_schematic.at(i))
                    continue;
                auto binding = inverse_bindings.find(predicate.arguments[i]);
                if (binding != inverse_bindings.end())
                    predicate = predicate.with_modified_argument(predicate.predicate_template.parameter_names.at(i),
                        binding->second);
            }
            auto origin = origins.find(predicate);
            if (origin == origins.end())
                continue;
            auto position = find(combined_facts.predicates.begin(), combined_facts.predicates.end(), predicate);
            if (position == combined_facts.predicates.end())
                throw logic_error("a construction phase has no canonical predicate");
            auto sources = get_support_sources(position - combined_facts.predicates.begin(), origin->second);
            witnesses.insert(sources.begin(), sources.end());
        }
        unify_references(bindings, {witnesses.begin(), witnesses.end()});
        refresh_conclusions();
    }
    else
    {
        conclusion_index = conclusions.size();
        conclusions.emplace_back();
    }
    fact_conclusions.push_back(conclusion_index);

    auto& conclusion = conclusions[conclusion_index];
    auto known = predicate_set(conclusion.expression);
    vector<Predicate> additions;
    for (const auto& predicate : canonicalize(facts.back().get_expression()).predicates)
        if (known.insert(predicate).second)
            additions.push_back(predicate);
    if (conclusion.phases.empty() || !additions.empty())
        conclusion.phases.push_back(KnowledgePhase{Expression(std::move(additions)), fact_index});
    refresh_conclusions();

    KnowledgeEvidence evidence{fact_index, {}, {}};
    for (const auto& reference : references(facts.back().get_expression()))
        evidence.reference_bindings.emplace(reference, canonical_reference(reference));
    map<Predicate, int> phases;
    for (size_t i = 0; i < conclusion.expression.predicates.size(); i++)
        phases.emplace(conclusion.expression.predicates[i], conclusion.predicate_phases[i]);
    for (const auto& predicate : facts.back().get_expression().predicates)
        evidence.predicate_phases.push_back(phases.at(canonicalize(predicate)));
    conclusion.evidence.push_back(std::move(evidence));

    refresh_predicates();
    set<string> changed_predicate_types;
    for (const auto& [predicate, producers] : predicate_producers)
    {
        if (reference_links.size() != previous_links)
        {
            changed_predicate_types.insert(predicate.predicate_template.predicate);
            continue;
        }
        for (const auto& producer : producers)
        {
            auto old = old_producers.find(predicate);
            if (old == old_producers.end() || !old->second.count(producer))
                changed_predicate_types.insert(predicate.predicate_template.predicate);
        }
    }
    return changed_predicate_types;
}

bool KnowledgeBase::find_conclusion(
    const Expression& expression, const KnowledgeSource& source,
    int& conclusion_index, map<string, string>& bindings) const
{
    if (try_get_conclusion(expression, conclusion_index, bindings))
        return true;

    auto incoming = predicate_set(expression);
    size_t best_size = 0;
    bool ambiguous_family = false;
    auto consider = [&](size_t index, size_t size, const map<string, string>& candidate_bindings)
    {
        if (size > best_size)
        {
            best_size = size;
            conclusion_index = index;
            bindings = candidate_bindings;
            ambiguous_family = false;
        }
        else if (size == best_size && conclusion_index != static_cast<int>(index))
            ambiguous_family = true;
    };
    for (size_t i = 0; i < conclusions.size(); i++)
    {
        const auto& conclusion = conclusions[i];
        bool has_lineage = false;
        for (int premise : source.source_indices)
            if (fact_conclusions.at(premise) == static_cast<int>(i))
                has_lineage = true;

        map<string, string> candidate_bindings;
        bool ambiguous = false;
        if (!conclusion.expression.predicates.empty()
            && conclusion.expression.try_match_topology(expression, candidate_bindings, TopologyMatch::SUBGRAPH,
                shared_bindings(conclusion.expression, expression), &ambiguous)
            && !ambiguous)
        {
            bool preserves_identity = all_of(candidate_bindings.begin(), candidate_bindings.end(),
                [](const auto& binding) { return binding.first == binding.second; });
            if (has_lineage || preserves_identity)
                consider(i, conclusion.expression.predicates.size(), candidate_bindings);
        }

        // Branching additions must share established identities; mere partial resemblance is insufficient.
        set<Predicate> prefix;
        for (const auto& phase : conclusion.phases)
        {
            auto normalized = canonicalize(phase.additions);
            prefix.insert(normalized.predicates.begin(), normalized.predicates.end());
            if (!prefix.empty() && includes(incoming.begin(), incoming.end(), prefix.begin(), prefix.end()))
            {
                Expression base(vector<Predicate>(prefix.begin(), prefix.end()));
                consider(i, prefix.size(), shared_bindings(base, base));
            }
        }
        auto current = predicate_set(conclusion.expression);
        if (!incoming.empty() && includes(current.begin(), current.end(), incoming.begin(), incoming.end()))
            consider(i, incoming.size(), shared_bindings(expression, expression));
    }
    if (best_size == 0 || ambiguous_family)
    {
        conclusion_index = -1;
        bindings.clear();
        return false;
    }
    return true;
}

bool KnowledgeBase::changes_conclusion(
    const Expression& expression, int conclusion_index,
    const map<string, string>& bindings) const
{
    for (const auto& [left, right] : bindings)
        if (canonical_reference(left) != canonical_reference(right))
            return true;
    auto known = predicate_set(conclusions.at(conclusion_index).expression);
    for (const auto& predicate : expression.predicates)
        if (!known.count(predicate))
            return true;
    return false;
}

string KnowledgeBase::canonical_reference(string reference) const
{
    auto link = reference_links.find(reference);
    while (link != reference_links.end())
    {
        reference = link->second.target;
        link = reference_links.find(reference);
    }
    return reference;
}

Predicate KnowledgeBase::canonicalize(Predicate predicate) const
{
    for (size_t i = 0; i < predicate.arguments.size(); i++)
        if (predicate.predicate_template.are_params_schematic.at(i))
            predicate = predicate.with_modified_argument(predicate.predicate_template.parameter_names.at(i),
                canonical_reference(predicate.arguments[i]));
    return predicate;
}

Expression KnowledgeBase::canonicalize(const Expression& expression) const
{
    vector<Predicate> predicates;
    set<Predicate> included;
    for (const auto& original : expression.predicates)
    {
        auto predicate = canonicalize(original);
        if (included.insert(predicate).second)
            predicates.push_back(std::move(predicate));
    }
    return Expression(std::move(predicates));
}

void KnowledgeBase::unify_references(const map<string, string>& bindings, const vector<int>& sources)
{
    for (const auto& [left, right] : bindings)
    {
        string first = canonical_reference(left);
        string second = canonical_reference(right);
        if (first == second)
            continue;
        if (reference_order.at(first) > reference_order.at(second))
            swap(first, second);
        reference_links.emplace(second, ReferenceLink{first, sources});
    }
}

void KnowledgeBase::refresh_conclusions()
{
    for (auto& conclusion : conclusions)
    {
        vector<Predicate> predicates;
        conclusion.predicate_phases.clear();
        set<Predicate> included;
        for (size_t i = 0; i < conclusion.phases.size(); i++)
            for (const auto& predicate : canonicalize(conclusion.phases[i].additions).predicates)
                if (included.insert(predicate).second)
                {
                    predicates.push_back(predicate);
                    conclusion.predicate_phases.push_back(i + 1);
                }
        conclusion.expression = Expression(std::move(predicates));
    }
}

void KnowledgeBase::refresh_predicates()
{
    map<Predicate, set<Producer>> normalized_producers;
    for (const auto& [predicate, producers] : predicate_producers)
    {
        auto& normalized = normalized_producers[canonicalize(predicate)];
        normalized.insert(producers.begin(), producers.end());
    }
    predicate_sources.clear();
    recorded_expressions.clear();
    vector<Predicate> predicates;
    for (size_t i = 0; i < facts.size(); i++)
    {
        auto normalized = canonicalize(facts[i].get_expression());
        if (evidence_mode == EvidenceMode::FIRST_PROOF)
            recorded_expressions.insert(predicate_set(normalized));
        for (const auto& predicate : normalized.predicates)
        {
            auto& sources = predicate_sources[predicate];
            if (sources.empty())
                predicates.push_back(predicate);
            sources.push_back(i);
            const auto& source = facts[i].get_source();
            normalized_producers[predicate].emplace(source.type, source.rule_name);
        }
    }
    predicate_producers = std::move(normalized_producers);
    combined_facts = Expression(std::move(predicates));
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

int KnowledgeBase::get_conclusion_index(int fact_index) const
{
    return fact_conclusions.at(fact_index);
}

bool KnowledgeBase::try_get_conclusion(
    const Expression& expression,
    int& conclusion_index,
    map<string, string>& reference_bindings) const
{
    auto normalized = canonicalize(expression);
    for (size_t i = 0; i < conclusions.size(); i++)
    {
        vector<Predicate> prefix;
        for (const auto& phase : conclusions[i].phases)
        {
            prefix.insert(prefix.end(), phase.additions.predicates.begin(), phase.additions.predicates.end());
            auto snapshot = canonicalize(Expression(prefix));
            bool ambiguous = false;
            if (snapshot.try_match_topology(normalized, reference_bindings, TopologyMatch::EXACT,
                    shared_bindings(snapshot, normalized), &ambiguous)
                && !ambiguous)
            {
                conclusion_index = i;
                return true;
            }
        }
    }
    conclusion_index = -1;
    reference_bindings.clear();
    return false;
}

bool KnowledgeBase::should_skip_proof(const Expression& expression, const string& rule_name) const
{
    if (evidence_mode != EvidenceMode::FIRST_PROOF)
        return false;
    set<Predicate> predicates;
    for (const auto& predicate : expression.predicates)
        predicates.insert(canonicalize(predicate));
    if (!recorded_expressions.count(predicates))
        return false;
    for (const auto& predicate : predicates)
        if (!predicate_producers.at(predicate).count({KnowledgeSourceType::INFERRED, rule_name}))
            return false;
    return true;
}

bool KnowledgeBase::has_derivation(const Expression& expression, const KnowledgeSource& source) const
{
    auto normalized = canonicalize(expression);
    auto predicates = predicate_set(normalized);
    if (evidence_mode == EvidenceMode::FIRST_PROOF && source.type == KnowledgeSourceType::INFERRED)
    {
        int conclusion_index;
        map<string, string> bindings;
        if (recorded_expressions.count(predicates)
            || (find_conclusion(normalized, source, conclusion_index, bindings)
                && !changes_conclusion(normalized, conclusion_index, bindings)))
        {
            for (const auto& predicate : normalized.predicates)
                if (!predicate_producers.at(predicate).count({source.type, source.rule_name}))
                    return false;
            return true;
        }
    }
    auto premises = predicate_set(canonicalize(Expression(source.premise_predicates)));
    for (const auto& fact : facts)
    {
        const auto& existing_source = fact.get_source();
        if (existing_source.type != source.type
            || existing_source.rule_name != source.rule_name)
            continue;

        if (predicates == predicate_set(canonicalize(fact.get_expression()))
            && premises == predicate_set(canonicalize(Expression(existing_source.premise_predicates))))
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
    return get_fact_indices(combined_predicate_index).front();
}

vector<int> KnowledgeBase::get_support_sources(int combined_predicate_index, int fact_index) const
{
    const auto& expected = combined_facts.predicates.at(combined_predicate_index);
    for (const auto& predicate : facts.at(fact_index).get_expression().predicates)
    {
        if (!(canonicalize(predicate) == expected))
            continue;
        set<int> sources{fact_index};
        for (size_t i = 0; i < predicate.arguments.size(); i++)
        {
            if (!predicate.predicate_template.are_params_schematic.at(i))
                continue;
            auto link = reference_links.find(predicate.arguments[i]);
            while (link != reference_links.end())
            {
                sources.insert(link->second.source_indices.begin(), link->second.source_indices.end());
                link = reference_links.find(link->second.target);
            }
        }
        return {sources.begin(), sources.end()};
    }
    throw logic_error("knowledge support does not contain its canonical premise");
}

const vector<int>& KnowledgeBase::get_fact_indices(int combined_predicate_index) const
{
    return predicate_sources.at(combined_facts.predicates.at(combined_predicate_index));
}

bool KnowledgeBase::has_independent_producer(int combined_predicate_index, const string& rule_name) const
{
    const auto& producers = predicate_producers.at(combined_facts.predicates.at(combined_predicate_index));
    return any_of(producers.begin(), producers.end(), [&](const Producer& producer)
    {
        return producer.first != KnowledgeSourceType::INFERRED || producer.second != rule_name;
    });
}

bool KnowledgeBase::query(
    const Expression& query_expression, const map<string, string>& fixed_bindings) const
{
    auto canonical_bindings = fixed_bindings;
    for (auto& [reference, target] : canonical_bindings)
        target = canonical_reference(target);
    map<string, string> bindings;
    return query_expression.try_match_topology(
        combined_facts, bindings, TopologyMatch::PATTERN, canonical_bindings);
}
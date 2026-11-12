#ifndef KNOWLEDGE_BASE_HPP
#define KNOWLEDGE_BASE_HPP

#include <vector>
#include <string>
#include <set>
#include <map>
#include <utility>
#include <deque>

#include "expression.hpp"

using namespace std;

class InferenceHandler;

enum class KnowledgeSourceType
{
    GIVEN = 0,
    INFERRED = 1,
};

class KnowledgeSource
{
public:
    KnowledgeSourceType type = KnowledgeSourceType::GIVEN;
    vector<int> source_indices;
    string rule_name;
    // Immutable premise snapshots; view indices can change when references unify.
    vector<Predicate> premise_predicates;
};

class Knowledge
{
private:
    Expression expression;
    KnowledgeSource source;
public:
    Knowledge(Expression expression, KnowledgeSource source)
        : expression(std::move(expression)), source(std::move(source)) {}

    const Expression& get_expression() const { return expression; }
    const KnowledgeSource& get_source() const { return source; }
};

class KnowledgeEvidence
{
public:
    int fact_index;
    // Original evidence references -> canonical references at registration.
    map<string, string> reference_bindings;
    // Construction phases supporting this record's predicates, not the whole latest graph.
    vector<int> predicate_phases;
};

class KnowledgePhase
{
public:
    Expression additions;
    int fact_index;
};

class KnowledgeConclusion
{
public:
    Expression expression;
    vector<int> predicate_phases;
    vector<KnowledgePhase> phases;
    vector<KnowledgeEvidence> evidence;
};

class KnowledgeBase
{
private:
    friend class InferenceHandler;

    InferenceHandler* inference_handler;

    vector<Knowledge> facts;
    vector<KnowledgeConclusion> conclusions;
    map<Predicate, vector<int>> predicate_sources;
    vector<int> fact_conclusions;

    struct ReferenceLink
    {
        string target;
        vector<int> source_indices;
    };
    map<string, ReferenceLink> reference_links;
    map<string, size_t> reference_order;

    Expression combined_facts;
    deque<string> inference_agenda;
    set<string> scheduled_rules;

    set<string> add_knowledge(Knowledge knowledge);
    bool find_conclusion(
        const Expression& expression, const KnowledgeSource& source,
        int& conclusion_index, map<string, string>& bindings) const;
    void refresh_conclusions();
    void refresh_predicates();
    void unify_references(const map<string, string>& bindings, const vector<int>& sources);
public:
    // A null handler stores facts for an explicit inference call instead.
    explicit KnowledgeBase(InferenceHandler* inference_handler);

    void tell_fact(Expression fact);
    // Store input immediately and leave inference for advance_inference().
    void enqueue_fact(Expression fact);
    bool has_pending_inferences() const;
    // Try one queued rule; false means no result this turn, not necessarily an empty agenda.
    bool advance_inference();
    const vector<Knowledge>& get_facts() const;
    const vector<KnowledgeConclusion>& get_conclusions() const;
    int get_conclusion_index(int fact_index) const;
    string canonical_reference(string reference) const;
    Predicate canonicalize(Predicate predicate) const;
    Expression canonicalize(const Expression& expression) const;
    bool try_get_conclusion(const Expression& expression, int& conclusion_index, map<string, string>& reference_bindings) const;
    bool has_derivation(const Expression& expression, const KnowledgeSource& source) const;
    const Expression& get_combined_facts() const;
    int get_fact_index(int combined_predicate_index) const;
    const vector<int>& get_fact_indices(int combined_predicate_index) const;
    vector<int> get_support_sources(int combined_predicate_index, int fact_index) const;
    vector<string> query(Expression query_expression);
};

#endif

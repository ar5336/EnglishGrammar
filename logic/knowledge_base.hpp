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
    // Premises in the knowledge base's combined predicate view.
    vector<int> predicate_indices;
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
    // Representative conclusion references -> this evidence's original references.
    map<string, string> reference_bindings;
};

class KnowledgeConclusion
{
public:
    Expression expression;
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

    vector<int> expression_index_list_by_combined_facts_index;
    Expression combined_facts;
    deque<string> inference_agenda;
    set<string> scheduled_rules;

    set<string> add_knowledge(Knowledge knowledge);
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
    bool try_get_conclusion(const Expression& expression, int& conclusion_index, map<string, string>& reference_bindings) const;
    bool has_derivation(const Expression& expression, const KnowledgeSource& source) const;
    const Expression& get_combined_facts() const;
    int get_fact_index(int combined_predicate_index) const;
    const vector<int>& get_fact_indices(int combined_predicate_index) const;
    vector<string> query(Expression query_expression);
};

#endif

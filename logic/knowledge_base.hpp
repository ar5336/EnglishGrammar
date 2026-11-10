#ifndef KNOWLEDGE_BASE_HPP
#define KNOWLEDGE_BASE_HPP

#include <vector>
#include <string>
#include <set>
#include <utility>

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

class KnowledgeBase
{
private:
    InferenceHandler* inference_handler;

    vector<Knowledge> facts;
    set<Predicate> known_predicates;

    vector<int> expression_index_list_by_combined_facts_index;
    Expression combined_facts;

    void add_knowledge(Knowledge knowledge);
public:
    // A null handler stores facts for an explicit inference call instead.
    explicit KnowledgeBase(InferenceHandler* inference_handler);

    void tell_fact(Expression fact);
    const vector<Knowledge>& get_facts() const;
    const Expression& get_combined_facts() const;
    int get_fact_index(int combined_predicate_index) const;
    vector<string> query(Expression query_expression);
};

#endif

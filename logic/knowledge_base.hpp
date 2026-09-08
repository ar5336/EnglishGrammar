#ifndef KNOWLEDGE_BASE_HPP
#define KNOWLEDGE_BASE_HPP

#include <vector>
#include <string>

#include "inference/inference_rule.hpp"
#include "expression.hpp"

using namespace std;

enum KnowledgeSourceType
{
    GIVEN = 0,
    INFERRED = 1,
};

class KnowledgeSource
{
public:
    KnowledgeSourceType type;
    vector<int> source_indices;
};

class Knowledge
{
private:
    Expression expression;
    KnowledgeSource source;
public:
    Knowledge(Expression expression, KnowledgeSource source)
        : expression(expression), source(source) {}
};

class KnowledgeBase
{
private:
    InferenceHandler* inference_handler;

    vector<pair<Expression, KnowledgeSource>> facts;

    vector<int> expression_index_list_by_combined_facts_index;
    Expression combined_facts;
public:
    KnowledgeBase(InferenceHandler* inference_handler);

    void tell_fact(Expression fact);
    vector<string> query(Expression query_expression);
};

#endif

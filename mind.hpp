#ifndef MIND_HPP
#define MIND_HPP

#include <string>
#include <map>
#include <utility>
#include <set>
#include <vector>

#include "predicate_handler.hpp"

enum KnowledgeType
{
    GIVEN,
    INFERRED,
};

enum ResponseType
{
    YES,
    NO,
};

class Mind
{
private:

    map<string, string> inheritance_map;
    set<string> noun_set;
    map<string, vector<Expression>> mentioned_nouns;

    set<Expression> given_expressions;
    set<Expression> inferred_expressions;

    void UpdateInheritanceMap();

    vector<string> IdentifyAllParents(string entityName);

public:
    Mind();

    vector<pair<KnowledgeType, Expression>> expressions;

    void InferExpressions();

    ResponseType DetermineResponse(Expression query_expression);

    void tell(Expression expression);
};

#endif
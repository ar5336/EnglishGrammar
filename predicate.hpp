#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <set>

#include "string_operators.hpp"

enum PredicateType
{
    IS_SUBSET_OF,
    IS_INSTANCE_OF,
    HAS_PROPERTY,
    CAN_DO,
    NONE,
};

enum SpeechActs
{
    STATEMENT,
    QUESTION,
    DEMAND
};

class Predicate
{
public:
    PredicateType type;
    vector<string> arguments;
    SpeechActs speechAct;

    Predicate();

    Predicate(PredicateType name, vector<string> arguments);

    Predicate(PredicateType name, vector<string> arguments, SpeechActs speechAct);

    string stringify();
};

inline bool operator<(const Predicate& lhs, const Predicate& rhs);

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

class PredicateHandler
{
private:
    map<string, string> inheritance_map;
    set<string> entity_set;
    map<string, vector<Predicate> > first_arg_to_predicate_map;
    // map<string, vector<Predicate>> entity_to_predicate_map;

    set<Predicate> given_predicates;
    set<Predicate> inferred_predicates;

    void UpdateInheritanceMap();

    vector<string> IdentifyAllParents(string entityName);
public:
    vector<pair<KnowledgeType, Predicate>> predicates;

    ResponseType DetermineResponse(Predicate queryPredicate);

    void add(Predicate predicate);

    PredicateHandler();

    void InferPredicates();
};

#endif
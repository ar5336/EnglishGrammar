#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include <string>
#include <vector>
#include <map>

#include "string_operators.hpp"

enum PredicateType
{
    IS_SUBSET_OF,
    IS_INSTANCE_OF,
    HAS_PROPERTY,
    CAN_DO,
    NONE,
};

class Predicate
{
public:
    PredicateType type;
    vector<string> arguments;

    Predicate();

    Predicate(PredicateType name, vector<string> arguments);

    string stringify();
};

class PredicateGroup
{
    vector<Predicate> predicates;
};

class PredicateHandler
{
public:
    vector<Predicate> predicates;

    PredicateHandler();
};

#endif
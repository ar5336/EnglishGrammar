// a predicate statement consists of a list of predicates
// a predicate has an operation and a number of arguments

// a predicate statement is not just a list though, it also can be turned into a graph so as to compare to other predicates
// checking for consistency and agreement

#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include <string>
#include <vector>

#include "string_operators.hpp"

enum PredicateType
{
    IS_SUBSET_OF = 1,
    IS_INSTANCE_OF = 2,
    HAS_PROPERTY = 3,
    CAN_DO = 4,
};

// enum StatementType
// {
//     Statement = 1,
//     Question = 2,
// };

class Predicate
{
public:
    string name;
    vector<string> arguments;

    Predicate(string name, vector<string> arguments);

    string to_string();
};

class PredicateGroup
{
    vector<Predicate> predicates;
};

// class PredicateStatement
// {
//     vector<Predicate> predicates;
// };

#endif
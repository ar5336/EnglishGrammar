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

const string predicateTypeNames[]
{
    "IS_SUBSET_OF",
    "IS_INSTANCE_OF",
    "HAS_PROPERTY",
    "CAN_DO",
    "NONE",
};

enum SpeechActs
{
    STATEMENT,
    QUESTION,
    DEMAND
};

class PredicateUtil
{
public:
    static string TypeToString(PredicateType type);

    static PredicateType StringToType(string string);
};

class Predicate
{
public:
    PredicateType type;
    vector<string> arguments;
    SpeechActs speech_act;

    Predicate();

    Predicate(PredicateType name, vector<string> arguments);

    Predicate(PredicateType name, vector<string> arguments, SpeechActs speechAct);

    string stringify();
};

bool operator<(const Predicate& lhs, const Predicate& rhs);

#endif
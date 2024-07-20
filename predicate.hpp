#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <set>

#include "string_operators.hpp"
#include "predicate_template.hpp"

// enum PredicateType
// {
//     IS_SUBSET_OF,
//     IS_INSTANCE_OF,
//     HAS_PROPERTY,
//     CAN_DO,
//     NONE,
// };

const string predicateTypeNames[]
{
    "IS_INSTANCE_OF",
    "HAS_PROPERTY",
    "PREPOSITION",
    "ACTION"
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
    static string TypeToString(int type_id);

    static int StringToTypeId(string string);
};

class Predicate
{
public:
    int type_id;
    PredicateTemplate predicate_template;
    vector<string> arguments;
    // SpeechActs speech_act;

    Predicate();

    Predicate(int type_id, vector<string> arguments);

    Predicate(int type_id, vector<string> arguments, SpeechActs speechAct);

    string stringify();

    string get_argument(string parameter_name);

    Predicate with_modified_argument(string paramenter_name, string new_value);
};

bool operator<(const Predicate& lhs, const Predicate& rhs);

bool operator==(const Predicate& lhs, const Predicate& rhs);

#endif
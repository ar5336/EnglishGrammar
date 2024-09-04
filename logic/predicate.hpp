#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <set>

#include "../string_operators.hpp"
#include "predicate_template.hpp"
#include "../global.hpp"

enum SpeechActs
{
    STATEMENT,
    QUESTION,
    DEMAND
};

class Predicate
{
public:
    int type_id;
    PredicateTemplate predicate_template;
    vector<string> arguments;
    // SpeechActs speech_act;

    Predicate();

    Predicate(int type_id, vector<string> arguments, PredicateTemplate predicate_template);

    Predicate(int type_id, vector<string> arguments, SpeechActs speechAct);

    string get_argument(string parameter_name);

    bool has_argument(string parameter_name);

    Predicate with_modified_argument(string paramenter_name, string new_value);
};

bool operator<(const Predicate& lhs, const Predicate& rhs);

bool operator==(const Predicate& lhs, const Predicate& rhs);

#endif
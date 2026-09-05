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

class PredicateArgumentAddress
{
public:
    PredicateArgumentAddress(
        int predicate_index,
        int statement_index,
        string predicate_type,
        string argument_name)
        : predicate_index(predicate_index),
        statement_index(statement_index),
        predicate_type(predicate_type),
        argument_name(argument_name) {}
    
    PredicateArgumentAddress(
        int predicate_index,
        string predicate_type,
        string argument_name)
        : predicate_index(predicate_index),
        statement_index(0),
        predicate_type(predicate_type),
        argument_name(argument_name) {}

    int predicate_index;
    int statement_index;
    string predicate_type;
    string argument_name;
};


class Predicate
{
public:
    int type_id;
    PredicateTemplate predicate_template;
    vector<string> arguments;
    map<string, int> argument_to_index_map;
    // SpeechActs speech_act;

    Predicate();

    Predicate(int type_id, vector<string> arguments, PredicateTemplate predicate_template);

    Predicate(int type_id, vector<string> arguments, SpeechActs speechAct);

    string get_argument(string parameter_name);

    bool try_get_argument(string parameter_name, string& out_value);

    bool has_argument(string parameter_name);

    Predicate with_modified_argument(string paramenter_name, string new_value);
};

bool operator<(const Predicate& lhs, const Predicate& rhs);

bool operator==(const Predicate& lhs, const Predicate& rhs);

#endif
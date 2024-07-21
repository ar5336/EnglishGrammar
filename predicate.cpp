#include "predicate.hpp"


Predicate::Predicate()
{
    type_id = -1;
    arguments = vector<string>(6);
    predicate_template = PredicateTemplate();
}

Predicate::Predicate(int type_id, vector<string> arguments, PredicateTemplate predicate_template)
    : type_id(type_id), arguments(arguments), predicate_template(predicate_template) {}

Predicate::Predicate(int type_id, vector<string> arguments, SpeechActs speechAct)
    : type_id(type_id), arguments(arguments) {}

string Predicate::stringify()
{
    string result;
    result += PredicateUtil::TypeToString(type_id);
    for (int argument_index = 0; argument_index < arguments.size(); argument_index++) {
        string arg = arguments[argument_index];
        string arg_name = predicate_template.parameter_names[argument_index];
        result += ( " " + arg_name + ":" + arg);
    }
    return result;
}

string Predicate::get_argument(string parameter_name)
{
    int param_index = predicate_template.parameter_index_map[parameter_name];

    return arguments[param_index];
}

string PredicateUtil::TypeToString(int type_id)
{
    return predicateTypeNames[type_id];
}

int PredicateUtil::StringToTypeId(string string)
{
    for (int i = 0; i < predicateTypeNames->size(); i++)
    {
        if (equals(string, predicateTypeNames[i]))
        {
            return i;
        }
    }
    return -1;
}

Predicate Predicate::with_modified_argument(string parameter_name, string new_value)
{
    // for(string argument_nam)
    int param_index = predicate_template.parameter_index_map[parameter_name];
    vector<string> modified_arguments = arguments;
    modified_arguments[param_index] = new_value;
    return Predicate(type_id, modified_arguments, predicate_template); 
}

bool operator<(const Predicate& lhs, const Predicate& rhs)
{
    return tie(lhs.type_id, lhs.arguments) < tie(rhs.type_id, rhs.arguments);
}

bool operator==(const Predicate& lhs, const Predicate& rhs)
{
    bool all_match = true;
    if (lhs.predicate_template.predicate != rhs.predicate_template.predicate)
        return false;

    if (lhs.arguments.size() != rhs.arguments.size())
        return false;

    for (int i = 0; i < lhs.arguments.size(); i++)
    {
        if (lhs.arguments[i] != rhs.arguments[i])
            return false;
    }

    return true;
}
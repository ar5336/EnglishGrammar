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

string Predicate::get_argument(string parameter_name)
{
    if (predicate_template.parameter_index_map.count(parameter_name) == 0)
    {
        throw runtime_error("can not find parameter name '"+parameter_name+"' in predicate of type '"+predicate_template.predicate+"'\n");
    }
    int param_index = predicate_template.parameter_index_map[parameter_name];

    return arguments[param_index];
}

bool Predicate::has_argument(string parameter_name)
{
    return predicate_template.parameter_index_map.count(parameter_name) != 0;
}

Predicate Predicate::with_modified_argument(string parameter_name, string new_value)
{
    if (predicate_template.parameter_index_map.count(parameter_name) == 0)
        throw runtime_error("bad parameter name for predicate template "+predicate_template.predicate);

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

    if (DEBUGGING)
        printf("they are both %s\n", lhs.predicate_template.predicate.c_str());

    if (lhs.arguments.size() != rhs.arguments.size())
        return false;

    if (DEBUGGING)
        printf("they both have args count of %ld\n", lhs.arguments.size());

    for (int i = 0; i < lhs.arguments.size(); i++)
    {
        string lhs_arg = lhs.arguments[i];
        string rhs_arg = rhs.arguments[i];

        if (!equals(lhs_arg, rhs_arg))
        {
            if (DEBUGGING)
                printf("mismatch between args '%s' and '%s'\n", lhs_arg.c_str(), rhs_arg.c_str());
            return false;
        }
    }

    return true;
}
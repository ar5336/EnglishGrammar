#include "predicate.hpp"

string get_pred_type_string(PredicateType pt)
{
    switch(pt) {
        case IS_SUBSET_OF: return "IS_SUBSET_OF";       break;
        case IS_INSTANCE_OF: return "IS_INSTANCE_OF";   break;
        case HAS_PROPERTY: return "HAS_PROPERTY";       break;
        case CAN_DO: return "CAN_DO";                   break;
        default:
            return "error";
    }
}

Predicate::Predicate()
{
    type = PredicateType::NONE;
    arguments = vector<string>(6);
}

Predicate::Predicate(PredicateType type, vector<string> arguments)
    : type(type), arguments(arguments), speech_act(SpeechActs::STATEMENT) {}

Predicate::Predicate(PredicateType type, vector<string> arguments, SpeechActs speechAct)
    : type(type), arguments(arguments), speech_act(speechAct){ }

string Predicate::stringify()
{
    string result;
    if (speech_act == SpeechActs::QUESTION) {
        result += "Q|";
    }
    result += get_pred_type_string(type);
    for (string arg : arguments) {
        result += (" " + arg);
    }
    return result;
}

string PredicateUtil::TypeToString(PredicateType type)
{
    return predicateTypeNames[(int)type];
}

PredicateType PredicateUtil::StringToType(string string)
{
    for (int i = 0; i < predicateTypeNames->size(); i++)
    {
        if (equals(string, predicateTypeNames[i]))
        {
            return (PredicateType)i;
        }
    }
    return PredicateType::NONE;
}

bool operator<(const Predicate& lhs, const Predicate& rhs)
{
    return tie(lhs.type, lhs.arguments) < tie(rhs.type, rhs.arguments);
}
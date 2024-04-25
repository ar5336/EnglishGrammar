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
}

Predicate::Predicate(PredicateType type, vector<string> arguments)
    : type(type), arguments(arguments){ }

string Predicate::stringify()
{
    string result = get_pred_type_string(type);
    for (string arg : arguments) {
        result += (" " + arg);
    }
    return result;
}

PredicateHandler::PredicateHandler(){}


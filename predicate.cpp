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
    : type(type), arguments(arguments), speechAct(SpeechActs::STATEMENT) {}

Predicate::Predicate(PredicateType type, vector<string> arguments, SpeechActs speechAct)
    : type(type), arguments(arguments), speechAct(speechAct){ }

string Predicate::stringify()
{
    string result;
    if (speechAct == SpeechActs::QUESTION) {
        result += "Q|";
    }
    result += get_pred_type_string(type);
    for (string arg : arguments) {
        result += (" " + arg);
    }
    return result;
}

inline bool operator<(const Predicate& lhs, const Predicate& rhs)
{
    return tie(lhs.type, lhs.arguments) < tie(rhs.type, rhs.arguments);
}

void PredicateHandler::UpdateInheritanceMap()
{
    for (auto pred_of_type : predicates)
    {
        if (pred_of_type.first == KnowledgeType::INFERRED)
            continue;
        
        auto pred = pred_of_type.second;

        // is a given predicate
        if (pred.type == PredicateType::IS_SUBSET_OF) {
            inheritance_map.emplace(pred.arguments[0], pred.arguments[1]);
        }

        for (string arg : pred.arguments)
        {
            entity_set.emplace(arg);
        }

        // populate the first_arg_to_predicate_map
        string first_arg = pred.arguments[0];

        if (first_arg_to_predicate_map.count(first_arg) != 0)
        {
            first_arg_to_predicate_map.at(first_arg).push_back(pred);
        } else {
            vector<Predicate> preds = {pred};
            first_arg_to_predicate_map.emplace(first_arg, preds);
        }
    }
}

ResponseType PredicateHandler::DetermineResponse(Predicate queryPredicate)
{
    // simple yes/no as of now
    auto asStatement = Predicate(queryPredicate.type, queryPredicate.arguments);

    auto is_given = given_predicates.count(asStatement) != 0;
    auto is_inferred = inferred_predicates.count(asStatement) != 0;

    if (is_given || is_inferred)
        {
            return ResponseType::YES;
        }

    return ResponseType::NO;
}

vector<string> PredicateHandler::IdentifyAllParents(string entity_name)
{
    // TODO - update inheritance_map to be <string> => vector<string>
    // so you can have multiple inheritance
    vector<string> parent_stack;

    set<string> visited_entities;
    string current_entity_name = entity_name;
    while (inheritance_map.count(current_entity_name) != 0
        && visited_entities.count(current_entity_name) == 0)
    {
        string parent = inheritance_map.at(current_entity_name);
        parent_stack.push_back(parent);
        visited_entities.emplace(current_entity_name);
        current_entity_name = parent;
        
    }
    return parent_stack;
}

void PredicateHandler::add(Predicate predicate)
{
    if ((given_predicates.emplace(predicate)).second)
    {
        predicates.push_back(pair(KnowledgeType::GIVEN, predicate));
    }

}

PredicateHandler::PredicateHandler(){
    // first_arg_to_predicate_map = map<string, vector<Predicate>>();
    
    add(Predicate(PredicateType::IS_SUBSET_OF, vector<string> {"horse", "mammal"}));
    add(Predicate(PredicateType::IS_SUBSET_OF, vector<string> {"bird", "animal"}));
    add(Predicate(PredicateType::IS_SUBSET_OF, vector<string> {"raven", "bird"}));
    add(Predicate(PredicateType::CAN_DO, vector<string> {"bird", "fly"}));
    
    InferPredicates();
}

void PredicateHandler::InferPredicates(){
    // first pass, build inheritance map
    UpdateInheritanceMap();

    for(string entity : entity_set)
    {
        auto parent_list = IdentifyAllParents(entity);
        
        for (string parent : parent_list) {
            if (first_arg_to_predicate_map.count(parent) != 0)
            {
                auto parent_predicates = first_arg_to_predicate_map.at(parent);

                for (auto parent_predicate : parent_predicates)
                {
                    auto new_tupe = parent_predicate.type;
                    auto changed_args = parent_predicate.arguments;
                    changed_args[0] = entity;

                    // TODO - find a more efficient way of not inferring certain already-inferred things
                    auto new_pred = Predicate(new_tupe, changed_args);
                    if (inferred_predicates.count(new_pred) == 0) {
                        // seemingly, the order here matters for some reason
                        auto was_inserted = inferred_predicates.insert(new_pred);
                        if (was_inserted.second) {
                            predicates.push_back(pair(KnowledgeType::INFERRED, new_pred));
                        }
                    }
                }
            }
        }
    }
}

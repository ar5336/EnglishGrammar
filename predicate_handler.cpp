#include "predicate_handler.hpp"
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

void PredicateHandler::tell(Predicate predicate)
{
    if ((given_predicates.emplace(predicate)).second)
    {
        predicates.push_back(pair(KnowledgeType::GIVEN, predicate));
    }

}

PredicateHandler::PredicateHandler(){
    // first_arg_to_predicate_map = map<string, vector<Predicate>>();
    
    tell(Predicate(PredicateType::IS_SUBSET_OF, vector<string> {"horse", "mammal"}));
    tell(Predicate(PredicateType::IS_SUBSET_OF, vector<string> {"bird", "animal"}));
    tell(Predicate(PredicateType::IS_SUBSET_OF, vector<string> {"raven", "bird"}));
    tell(Predicate(PredicateType::CAN_DO, vector<string> {"bird", "fly"}));
    
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

Predicate PredicateHandler::PredFromString(string input)
{
    auto tokens = split_spaces(input);

    PredicateType type = PredicateUtil::StringToType(tokens[0]);
    if (type != PredicateType::NONE)
    {
        auto arguments = vector<string>(tokens.begin()+1, tokens.end());
        return Predicate(type, arguments);
    }
    return Predicate(PredicateType::NONE, vector<string>());
}
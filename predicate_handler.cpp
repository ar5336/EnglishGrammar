#include "predicate_handler.hpp"
void PredicateHandler::UpdateInheritanceMap()
{
    // for (auto pred_of_type : expressions)
    // {
    //     if (pred_of_type.first == KnowledgeType::INFERRED)
    //         continue;
        
    //     auto pred = pred_of_type.second;

    //     // is a given predicate
    //     if (pred.type == PredicateType::IS_SUBSET_OF) {
    //         inheritance_map.emplace(pred.arguments[0], pred.arguments[1]);
    //     }

    //     for (string arg : pred.arguments)
    //     {
    //         entity_set.emplace(arg);
    //     }

    //     // populate the first_arg_to_predicate_map
    //     string first_arg = pred.arguments[0];

    //     if (first_arg_to_predicate_map.count(first_arg) != 0)
    //     {
    //         first_arg_to_predicate_map.at(first_arg).push_back(pred);
    //     } else {
    //         vector<Predicate> preds = {pred};
    //         first_arg_to_predicate_map.emplace(first_arg, preds);
    //     }
    // }
}

ResponseType PredicateHandler::DetermineResponse(Expression query_expression)
{
    // simple yes/no as of now
    // auto asStatement = Predicate(queryPredicate.type_id, queryPredicate.arguments);

    auto is_given = given_expressions.count(query_expression) != 0;
    auto is_inferred = inferred_expressions.count(query_expression) != 0;

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

void PredicateHandler::tell(Expression expression)
{
    if ((given_expressions.emplace(expression)).second)
    {
        expressions.push_back(pair(KnowledgeType::GIVEN, expression));
    }
}

PredicateHandler::PredicateHandler(PredicateTemplateHandler *predicate_template_reader){
    // first_arg_to_predicate_map = map<string, vector<Predicate>>();
    
    predicate_template_reader = predicate_template_reader;

    // tell(ConstructPredicate("IS_SUBSET_OF", vector<string> {"horse", "mammal"}));
    // tell(ConstructPredicate("IS_SUBSET_OF", vector<string> {"bird", "animal"}));
    // tell(ConstructPredicate("IS_SUBSET_OF", vector<string> {"raven", "bird"}));
    // tell(ConstructPredicate("CAN_DO", vector<string> {"bird", "fly"}));
    
    InferExpressions();
}

void PredicateHandler::InferExpressions(){
    // first pass, build inheritance map
    UpdateInheritanceMap();

    // for(string entity : entity_set)
    // {
    //     auto parent_list = IdentifyAllParents(entity);
        
    //     for (string parent : parent_list) {
    //         if (first_arg_to_predicate_map.count(parent) != 0)
    //         {
    //             auto parent_predicates = first_arg_to_predicate_map.at(parent);

    //             for (auto parent_predicate : parent_predicates)
    //             {
    //                 auto new_tupe = parent_predicate.type;
    //                 auto changed_args = parent_predicate.arguments;
    //                 changed_args[0] = entity;

    //                 // TODO - find a more efficient way of not inferring certain already-inferred things
    //                 auto new_pred = Predicate(new_tupe, changed_args);
    //                 if (inferred_predicates.count(new_pred) == 0) {
    //                     // seemingly, the order here matters for some reason
    //                     auto was_inserted = inferred_predicates.insert(new_pred);
    //                     if (was_inserted.second) {
    //                         predicates.push_back(pair(KnowledgeType::INFERRED, new_pred));
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }
}

int PredicateHandler::PredIntFromString(string type)
{
    return predicate_template_handler->GetPredicateIndex(type);
}


Predicate PredicateHandler::PredFromString(string input)
{
    auto tokens = split_spaces(input);

    int type_id = PredicateUtil::StringToTypeId(tokens[0]);
    if (type_id != -1)
    {
        auto arguments = vector<string>(tokens.begin()+1, tokens.end());
        return Predicate(type_id, arguments);
    }
    return Predicate(type_id, vector<string>());
}

Predicate PredicateHandler::ConstructPredicate(string predicate_name, vector<string> predicate_arguments)
{
    PredicateTemplate predicate_template = predicate_template_handler->GetPredicateTemplate(predicate_name);

    if (predicate_arguments.size() != predicate_template.parameter_names.size())
        throw "this predicate is malformed";

    int type_index = predicate_template_handler->GetPredicateIndex(predicate_name);
    
    return Predicate(type_index, predicate_arguments);
}

PredicateTemplate PredicateHandler::GetPredicateTemplate(string predicate_name)
{
    return predicate_template_handler->GetPredicateTemplate(predicate_name);
}

bool operator<(const Expression& lhs, const Expression& rhs)
{
    return tie(lhs.predicates) < tie(rhs.predicates);
}

Expression::Expression() {}

Expression::Expression(vector<Predicate> predicates) : predicates(predicates) {}

Expression Expression::combine_expressions(Expression expression1, Expression expression2)
{
    vector<Predicate> total_predicates = expression1.predicates;
    vector<Predicate> predicates_2 = expression2.predicates;

    for (Predicate pred : predicates_2)
    {
        total_predicates.push_back(pred);
    }

    return Expression(total_predicates);
}

Predicate Expression::get_predicate_by_name(Expression expression, string predicate_name)
{
    for (Predicate pred : expression.predicates)
    {
        if (pred.predicate_template.predicate == predicate_name)
            return pred;
    }
    return Predicate();
}

Predicate Expression::extract_predicate(Predicate original)
{
    vector<Predicate> leftover_predicates;

    Predicate extracted_predicate;
    bool predicate_found;

    for (auto pred : predicates)
    {
        if (pred == original && !predicate_found)
        {
            extracted_predicate = original;
            predicate_found = true;
            continue;
        }

        leftover_predicates.push_back(pred);
    }

    if (predicate_found)
    {
        predicates = leftover_predicates;
        return extracted_predicate;
    }

    return Predicate();
}

string Expression::stringify()
{
    string accumulated_string = "";

    for (auto predicate : predicates)
    {
        accumulated_string += predicate.stringify();
        accumulated_string += "\n";
    }

    return accumulated_string;
}
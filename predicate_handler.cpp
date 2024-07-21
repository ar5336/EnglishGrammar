#include "predicate_handler.hpp"

PredicateHandler::PredicateHandler(PredicateTemplateHandler *predicate_template_reader){
    // first_arg_to_predicate_map = map<string, vector<Predicate>>();
    
    predicate_template_handler = predicate_template_reader;

    // tell(ConstructPredicate("IS_SUBSET_OF", vector<string> {"horse", "mammal"}));
    // tell(ConstructPredicate("IS_SUBSET_OF", vector<string> {"bird", "animal"}));
    // tell(ConstructPredicate("IS_SUBSET_OF", vector<string> {"raven", "bird"}));
    // tell(ConstructPredicate("CAN_DO", vector<string> {"bird", "fly"}));
    
    // InferExpressions();
}

int PredicateHandler::PredIntFromString(string type)
{
    return predicate_template_handler->GetPredicateIndex(type);
}


Predicate PredicateHandler::PredFromString(string input)
{
    auto tokens = split_spaces(input);

    int type_id = StringToTypeId(tokens[0]);
    PredicateTemplate input_template;
    predicate_template_handler->try_get_predicate_template(input, &input_template);
    if (type_id != -1)
    {
        auto arguments = vector<string>(tokens.begin()+1, tokens.end());
        return Predicate(type_id, arguments, input_template);
    }
    return Predicate(type_id, vector<string>(), input_template);
}

Predicate PredicateHandler::ConstructPredicate(string predicate_name, vector<string> predicate_arguments)
{
    PredicateTemplate predicate_template = PredicateTemplate();
    if (!predicate_template_handler->try_get_predicate_template(predicate_name, &predicate_template))
        throw runtime_error("predicate name '" + predicate_name + "' is wrong");

    if (predicate_arguments.size() != predicate_template.parameter_names.size())
        throw runtime_error("this predicate is malformed");

    int type_index = predicate_template_handler->GetPredicateIndex(predicate_name);

    return Predicate(type_index, predicate_arguments, predicate_template);
}

bool PredicateHandler::try_get_predicate_template(string predicate_name, PredicateTemplate *predicate_template)
{
    return predicate_template_handler->try_get_predicate_template(predicate_name, predicate_template);
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

string PredicateHandler::stringify_expression(Expression expression)
{
    string accumulated_string = "";

    for (auto predicate : expression.predicates)
    {
        accumulated_string += stringify_predicate(predicate);
        accumulated_string += "\n";
    }

    return accumulated_string;
}

void PredicateHandler::init_stringification(){
    predicate_type_names = vector<string>(predicate_template_handler->predicate_types.begin(), predicate_template_handler->predicate_types.end());
}


string PredicateHandler::TypeToString(int type_id)
{
    if (type_id < 0)
        throw runtime_error("index "+to_string(type_id)+" out of range.");
    return predicate_type_names[type_id];
}

int PredicateHandler::StringToTypeId(string string)
{
    for (int i = 0; i < predicate_type_names.size(); i++)
    {
        if (equals(string, predicate_type_names[i]))
        {
            return i;
        }
    }
    return -1;
}

string PredicateHandler::stringify_predicate(Predicate predicate)
{
    string result;
    result += predicate_type_names.at(predicate.type_id);
    for (int argument_index = 0; argument_index < predicate.arguments.size(); argument_index++) {
        string arg = predicate.arguments[argument_index];
        string arg_name = predicate.predicate_template.parameter_names[argument_index];
        result += ( " " + arg_name + ":" + arg);
    }
    return result;
}

// string PredicateHandler::StringifyPredicate(Predicate predicate)
// {
//     string result;
//     result += TypeToString(predicate.type_id);
//     for (int argument_index = 0; argument_index < predicate.arguments.size(); argument_index++) {
//         string arg = predicate.arguments[argument_index];
//         string arg_name = predicate.predicate_template.parameter_names[argument_index];
//         result += ( " " + arg_name + ":" + arg);
//     }
//     return result;
// }
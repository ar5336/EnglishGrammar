#include "predicate_handler.hpp"

PredicateHandler::PredicateHandler(PredicateTemplateHandler *predicate_template_reader){
    // first_arg_to_predicate_map = map<string, vector<Predicate>>();
    
    predicate_template_handler = predicate_template_reader;

    // tell(construct_predicate("IS_SUBSET_OF", vector<string> {"horse", "mammal"}));
    // tell(construct_predicate("IS_SUBSET_OF", vector<string> {"bird", "animal"}));
    // tell(construct_predicate("IS_SUBSET_OF", vector<string> {"raven", "bird"}));
    // tell(construct_predicate("CAN_DO", vector<string> {"bird", "fly"}));
    
    // InferExpressions();
}

int PredicateHandler::pred_int_from_string(string type)
{
    return predicate_template_handler->get_predicate_index(type);
}


Predicate PredicateHandler::pred_from_string(string input)
{
    auto tokens = split_spaces(input);

    int type_id = string_to_type_id(tokens[0]);
    PredicateTemplate input_template;
    predicate_template_handler->try_get_predicate_template(input, &input_template);
    if (type_id != -1)
    {
        auto arguments = vector<string>(tokens.begin()+1, tokens.end());
        return Predicate(type_id, arguments, input_template);
    }
    return Predicate(type_id, vector<string>(), input_template);
}

Predicate PredicateHandler::construct_predicate(string predicate_name, vector<string> predicate_arguments)
{
    if (DEBUGGING)
    {
        printf("creating predicate: %s", predicate_name.c_str());
        for(string argument : predicate_arguments)
        {
            printf(" %s", argument.c_str());
        }
        printf("\n");
    }
    PredicateTemplate predicate_template = PredicateTemplate();
    if (!predicate_template_handler->try_get_predicate_template(predicate_name, &predicate_template))
        throw runtime_error("predicate name '" + predicate_name + "' is wrong");

    if (predicate_arguments.size() != predicate_template.parameter_names.size())
        throw runtime_error("this predicate is malformed");

    int type_index = predicate_template_handler->get_predicate_index(predicate_name);

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

Expression::Expression() {
    prid_to_prid_by_arg = map<int, map<int, tuple<string, string>>>();

}

Expression::Expression(vector<Predicate> predicates) : predicates(predicates) {
    prid_to_prid_by_arg = map<int, map<int, tuple<string, string>>>();
    // construct noun set
    noun_set = set<string>();
    for (auto predicate : predicates)
    {
        if (predicate.has_argument("noun_class"))
        {
            noun_set.emplace(predicate.get_argument("noun_class"));
        }
    }
    make_connections();
}

vector<Predicate> Expression::get_predicates()
{
    return predicates;
}

// vector<Predicate> Expression::add_predicate(Predicate predicate)
// {
//     predicates.push_back(predicate);
//     for (int i = 0; i < 0; i < predicates.size(); i++)
//     {
//         for (int i = 0; )
//         make_connection()
//     }
// }

void Expression::make_connections()
{
    for (int i = 0; i < predicates.size(); i++)
    {
        string predicate_type = predicates[i].predicate_template.predicate;

        if (prids_by_type.count(predicate_type) == 0)
        {
            prids_by_type.emplace(predicate_type, vector<int>{i});
        } else {
            prids_by_type.at(predicate_type).push_back(i);
        }
    }
    // for each predicate, index up their variables.
    map<string, vector<int>> arg_name_to_prid;
    for (int prid = 0; prid < predicates.size(); prid++)
    {
        Predicate predicate = predicates[prid];
        for (string arg_name : predicate.predicate_template.parameter_names)
        {
            if (arg_name_to_prid.count(arg_name) != 0)
            {
                arg_name_to_prid.at(arg_name).push_back(prid);
            } else {
                arg_name_to_prid.emplace(arg_name, vector<int> {prid});
            }
        }
    }
    for (int i = 0; i < predicates.size(); i++)
    {
        auto predicate = predicates[i];

        // the kinds of connections that can be made between predicates.

        // for the example cats are mammals
        // noun map

        for (int j = 0; j < predicates.size(); j++)
        {
            if (j == i) break;

            auto other_predicate = predicates[j];

            for (int k = 0; k < predicate.arguments.size(); k++)
            {

                for (int l = 0; l < other_predicate.arguments.size(); l++)
                {
                    auto var_1 = predicate.arguments[k];
                    auto var_2 = other_predicate.arguments[l];

                    if (equals(var_1, var_2))
                    {
                        string arg_1 = predicate.predicate_template.parameter_names[k];
                        string arg_2 = other_predicate.predicate_template.parameter_names[l];
                        add_connection(i, arg_1,
                                        j, arg_2);
                        add_connection(j, arg_2,
                                        i, arg_1);

                    }
                }
            }
        }

        
    }
}

void Expression::add_connection(int prid_1, string arg_1, int prid_2, string arg_2)
{
    // string var_1 = predicates[prid_1].get_argument(arg_1);
    // string var_2 = predicates[prid_2].get_argument(arg_2);

    // if (!equals(var_1, var_2))
    //     throw runtime_error("predicate ids '"+to_string(prid_1)+"' and '"+to_string(prid_2)+"' are bad");

    // if (prid_to_prid_by_arg.count(prid_1) == 0)
    // {
    //     prid_to_prid_by_arg.emplace(prid_1, tuple<int, string, string>(prid_2, arg_1, arg_2));
    // }
    // else {
    //     printf("failed to add connection");
    // }

    if (prid_to_prid_by_arg.count(prid_1) == 0)
    {
        // must create new connection + map
        auto new_map = map<int, tuple<string, string>>();
        new_map.emplace(prid_2, make_tuple(arg_1, arg_2));

        prid_to_prid_by_arg.emplace(prid_1, new_map);
    } else
    {
        auto to_map = prid_to_prid_by_arg.at(prid_1);

        if (to_map.count(prid_2) != 0)
            throw runtime_error("connection between these predicates already exists");

        to_map.emplace(prid_2, make_tuple(arg_1, arg_2));

        prid_to_prid_by_arg.erase(prid_1);

        prid_to_prid_by_arg.emplace(prid_1, to_map);
    }
}

Predicate Expression::operator [](int i) const
{
    return predicates[i];
}


tuple<bool, vector<tuple<int, int>>> Expression::has_connection(string pred_1, string arg_1, string pred_2, string arg_2)
{
    // if (DEBUGGING)
    //     printf("getting connections from\n\tpred:'%s' arg:'%s' to\n\tpred:'%s' arg:'%s'\n",
    //     source_predicate_type.c_str(),
    //     source_argument.c_str(),
    //     target_predicate_type.c_str(),
    //     target_argument.c_str());

    if (prids_by_type.count(pred_1) == 0 || prids_by_type.count(pred_2) == 0)
        return make_tuple(false, vector<tuple<int, int>>());

    vector<int> candidate_prid_1s = prids_by_type.at(pred_1);
    vector<int> candidate_prid_2s = prids_by_type.at(pred_2);

    vector<tuple<int, int>> found_connections = vector<tuple<int, int>>();
    for (int candidate_prid_1 : candidate_prid_1s)
    {
        if (prid_to_prid_by_arg.count(candidate_prid_1) == 0)
            continue;

        for (int candidate_prid_2 : candidate_prid_2s)
        {
            auto candidiate_connection = prid_to_prid_by_arg.at(candidate_prid_1);

            if (candidiate_connection.count(candidate_prid_2) != 0)
            {
                // add to found connections
                auto connection_info = candidiate_connection.at(candidate_prid_2);

                string found_arg_1 = get<0>(connection_info);
                if (!equals(found_arg_1, arg_1))
                    continue;
                string found_arg_2 = get<1>(connection_info);
                if (!equals(found_arg_2, arg_2))
                    continue;

                found_connections.push_back(make_tuple(candidate_prid_1, candidate_prid_2));
            } else
                continue;
        }
    }

    if (found_connections.size() == 0)
        return make_tuple(false, found_connections);
    else
        return make_tuple(true, found_connections);
}

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

vector<pair<Predicate, Predicate>> Expression::get_connections(
    string source_predicate_type,
    string source_argument,
    string target_predicate_type,
    string target_argument)
{
    if (DEBUGGING)
        printf("getting connections from\n\tpred:'%s' arg:'%s' to\n\tpred:'%s' arg:'%s'\n",
        source_predicate_type.c_str(),
        source_argument.c_str(),
        target_predicate_type.c_str(),
        target_argument.c_str());

    auto has_connection_and_connection_ids =
        has_connection(
            source_predicate_type,
            source_argument,
            target_predicate_type,
            target_argument);
    
    bool has_connection = get<0>(has_connection_and_connection_ids);
    if (!has_connection)
        return vector<pair<Predicate, Predicate>>();
    
    auto predicate_index_pairs = get<1>(has_connection_and_connection_ids);

    auto predicate_pairs = vector<pair<Predicate, Predicate>>();

    for (auto predicate_index_pair : predicate_index_pairs)
    {
        int predicate_index_1 = get<0>(predicate_index_pair);
        int predicate_index_2 = get<1>(predicate_index_pair);

        predicate_pairs.push_back(make_pair(predicates[predicate_index_1], predicates[predicate_index_2]));
    }
    
    // dereference predicates by indeces
    return predicate_pairs;
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


string PredicateHandler::type_to_string(int type_id)
{
    if (type_id < 0)
        throw runtime_error("index "+to_string(type_id)+" out of range.");
    return predicate_type_names[type_id];
}

int PredicateHandler::string_to_type_id(string string)
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
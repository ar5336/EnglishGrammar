#include "predicate_handler.hpp"

PredicateHandler::PredicateHandler(PredicateTemplateHandler *predicate_template_reader){
    predicate_template_handler = predicate_template_reader;
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
        printf("\033[1;33mcreating\033[0m predicate: %s", predicate_name.c_str());
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
            string noun = predicate.get_argument("noun_class");
            noun_set.insert(noun);
        }
    }
    make_connections();
}

vector<Predicate> Expression::get_predicates()
{
    return predicates;
}

void Expression::make_connections()
{
    if (DEBUGGING)
        printf("making connections\n");

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
        {
            if (DEBUGGING)
            {
                printf("connection between predicate '%s' and predicate '%s' already exists\n",
                    predicates[prid_1].predicate_template.predicate.c_str(),
                    predicates[prid_2].predicate_template.predicate.c_str());
            }

            return;
        }

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

            string type_1 =  predicates[candidate_prid_1].predicate_template.predicate;
            string type_2 =  predicates[candidate_prid_2].predicate_template.predicate;

            if (!equals(type_1, pred_1) || !equals(type_2, pred_2))
            {
                printf("mismatch between 1 '%s' and 2 '%s'\n", type_1.c_str(), type_2.c_str());
                continue;
            }
            
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

    if (DEBUGGING)
        printf("expression combination initialized\n");
    
    for (Predicate pred : predicates_2)
    {
        total_predicates.push_back(pred);
    }

    if (DEBUGGING)
        printf("expression combination ended\n");

    return Expression(total_predicates);
}

bool Expression::try_get_predicates_by_name(Expression expression, string predicate_name, vector<Predicate> &result_predicates)
{
    result_predicates = vector<Predicate>();
    for (Predicate pred : expression.predicates)
    {
        if (equals(pred.predicate_template.predicate, predicate_name))
        {
            result_predicates.push_back(pred);
        }
    }
    return result_predicates.size() > 0;
}

Predicate Expression::extract_predicate(Predicate original)
{
    vector<Predicate> leftover_predicates = vector<Predicate>();

    Predicate extracted_predicate;
    bool predicate_found = false;

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

vector<Predicate> Expression::extract_predicate_types(Expression& og_expression, set<string> predicate_types)
{
    vector<Predicate> removed_predicates = vector<Predicate>();

    vector<Predicate> og_predicates = og_expression.predicates;
    for (auto predicate : og_predicates)
    {
        if (predicate_types.count(predicate.predicate_template.predicate) != 0)
        {
            removed_predicates.push_back(predicate);
            og_expression.extract_predicate(predicate);
        }
    }

    return removed_predicates;
}

vector<Predicate> Expression::extract_predicates_by_argument(Expression &og_expression, string argument, bool anaphorics_prohibited = false)
{
    vector<Predicate> removed_predicates = vector<Predicate>();
    vector<Predicate> list = og_expression.predicates;
    for (auto predicate : list)
    {
        if (anaphorics_prohibited && equals(predicate.predicate_template.predicate, "ANAPHORIC"))
            continue;
        
        for (int argument_index = 0; argument_index < predicate.arguments.size(); argument_index++)
        {
            string candidate_argument = predicate.arguments[argument_index];

            if (equals(candidate_argument, argument))
            {
                removed_predicates.push_back(og_expression.extract_predicate(predicate));
            }
        }
    }
    return removed_predicates;
}

vector<Predicate> Expression::extract_anaphora_closure_by_argument(Expression &og_expression, string argument)
{
    vector<Predicate> removed_predicates;

    set<Predicate> visited_predicates = set<Predicate>();
    vector<Predicate> predicates_to_visit = extract_predicates_by_argument(og_expression, argument, /*anaphorics prohibited*/ true);

    while(predicates_to_visit.size() > 0)
    {
        auto top_predicate = predicates_to_visit.back();
        predicates_to_visit.pop_back();

        og_expression.extract_predicate(top_predicate);
        removed_predicates.push_back(top_predicate);

        set<Predicate> relevant_predicates;
        
        for (int i = 0; i < top_predicate.arguments.size(); i++)
        {
            string top_predicate_argument = top_predicate.arguments[i];
            string top_predicate_param_name = top_predicate.predicate_template.parameter_names[i];

            if (DEBUGGING)
                printf("top argument: %s in %s\n", top_predicate_argument.c_str(), top_predicate.predicate_template.predicate.c_str());

            if (!top_predicate.predicate_template.is_param_schematic(top_predicate_param_name))
            {
                visited_predicates.emplace(top_predicate);
                continue;
            }

            auto identified_relevant_predicates = extract_predicates_by_argument(og_expression, top_predicate_argument, /*anaphorics prohibited*/ true);

            if (DEBUGGING)
            {
                printf("'%ld' relevant predicates identified\n", identified_relevant_predicates.size());
                // identified_relevant_predicates
            }
            for (auto identified_relevant_predicate : identified_relevant_predicates)
            {
                if (visited_predicates.count(identified_relevant_predicate) == 0)
                    relevant_predicates.emplace(identified_relevant_predicate);
            }
        }

        for (auto relevant_predicate : relevant_predicates)
        {
            predicates_to_visit.push_back(relevant_predicate);
        }

        visited_predicates.emplace(top_predicate);
    }

    if (DEBUGGING)
        printf("removing %ld predicates\n", removed_predicates.size());

    return removed_predicates;
}

vector<pair<Predicate, Predicate>> Expression::get_connections(
    string source_predicate_type,
    string source_argument,
    string target_predicate_type,
    string target_argument)
{
    if (DEBUGGING)
        printf("\033[1;34mgetting\033[0m connections from\n\tpred:'%s' arg:'%s' to\n\tpred:'%s' arg:'%s'\n",
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
    if (!has_connection){
        if (DEBUGGING)
            printf("no connections found\n");
        return vector<pair<Predicate, Predicate>>();
    }
    
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
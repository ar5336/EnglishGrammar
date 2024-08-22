#ifndef PREDICATE_TEMPLATE_HPP
#define PREDICATE_TEMPLATE_HPP

#include <map>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

class PredicateTemplate
{
public:
    string predicate;
    vector<string> parameter_names;
    vector<bool> are_params_schematic;

    map<string, int> parameter_index_map;

    PredicateTemplate();

    PredicateTemplate(string predicate_name, vector<string> parameter_names, vector<bool> are_params_schematic);

    bool contains_parameter_name(string parameter_name);

    bool is_param_schematic(string param_name);

    void replace(PredicateTemplate other);
};

class PredicateTemplateHandler
{
private:
    map<string, PredicateTemplate> predicate_templates;
    map<string, int> predicate_index_map;

    int running_index = 0;

public:
    vector<string> predicate_types;

    PredicateTemplateHandler();

    void add_entry(PredicateTemplate predicate_template);

    bool try_get_predicate_template(string predicate_name, PredicateTemplate* predicate_template);

    int get_predicate_index(string predicate_name);
};

#endif
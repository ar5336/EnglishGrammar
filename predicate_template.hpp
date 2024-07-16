#ifndef PREDICATE_TEMPLATE_HPP
#define PREDICATE_TEMPLATE_HPP

#include <map>
#include <string>
#include <vector>

using namespace std;

class PredicateTemplate
{
public:
    string predicate;
    vector<string> parameter_names;

    map<string, int> parameter_index_map;

    PredicateTemplate();

    PredicateTemplate(string predicate_name, vector<string> parameter_names);
};

class PredicateTemplateHandler
{
private:
    map<string, PredicateTemplate> predicate_templates;
    map<string, int> predicate_index_map;

    int running_index = 0;

public:
    PredicateTemplateHandler();

    void add_entry(PredicateTemplate predicate_template);

    PredicateTemplate GetPredicateTemplate(string predicate_name);

    int GetPredicateIndex(string predicate_name);
};

#endif
#include "predicate_template.hpp"

PredicateTemplate::PredicateTemplate() {}

PredicateTemplate::PredicateTemplate(string predicate_name, vector<string> parameter_names)
    : predicate(predicate_name), parameter_names(parameter_names) {}


void PredicateTemplateHandler::add_entry(PredicateTemplate predicate_template)
{
    predicate_types.push_back(predicate_template.predicate);
    predicate_templates.emplace(predicate_template.predicate, predicate_template);
    predicate_index_map.emplace(predicate_template.predicate, running_index);
    running_index++;
}

void PredicateTemplate::replace(PredicateTemplate other)
{
    predicate = other.predicate;
    parameter_names = other.parameter_names;
    parameter_index_map = other.parameter_index_map;
}

PredicateTemplateHandler::PredicateTemplateHandler() {
    predicate_types = vector<string>();
}

bool PredicateTemplateHandler::try_get_predicate_template(string predicate_name, PredicateTemplate* predicate_template)
{
    if (predicate_templates.count(predicate_name) != 0)
    {
        predicate_template->replace(predicate_templates.at(predicate_name));
        return true;
    }
    // printf("predicate_templates size: %d\n", (int)predicate_templates.size());
    // printf("predicate_templates 0: %s\n", predicate_templates[0].c_str());
    // printf("predicate_templates 1: %s\n", predicate_templates[1].c_str());
    printf("disaster: fetching failed\n");
    predicate_template->replace(PredicateTemplate());
    return false;
}

int PredicateTemplateHandler::get_predicate_index(string predicate_name)
{
    if (predicate_index_map.count(predicate_name) != 0)
    {
        return predicate_index_map.at(predicate_name);
    }
    printf("disaster: index fetching failed\n");
    return -1;
}
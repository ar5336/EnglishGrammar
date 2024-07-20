#include "predicate_template.hpp"

PredicateTemplate::PredicateTemplate() {}

PredicateTemplate::PredicateTemplate(string predicate_name, vector<string> parameter_names)
    : predicate(predicate_name), parameter_names(parameter_names) {}


void PredicateTemplateHandler::add_entry(PredicateTemplate predicate_template)
{
    predicate_templates.emplace(predicate_template.predicate, predicate_template);
    predicate_index_map.emplace(predicate_template.predicate, running_index);
    running_index++;
}

void PredicateTemplate::Replace(PredicateTemplate other)
{
    predicate = other.predicate;
    parameter_names = other.parameter_names;
    parameter_index_map = other.parameter_index_map;
}

PredicateTemplateHandler::PredicateTemplateHandler() {}

bool PredicateTemplateHandler::try_get_predicate_template(string predicate_name, PredicateTemplate* predicate_template)
{
    if (predicate_templates.count(predicate_name) != 0)
    {
        predicate_template->Replace(predicate_templates.at(predicate_name));
        return true;
    }
    // printf("predicate_templates size: %d\n", (int)predicate_templates.size());
    // printf("predicate_templates 0: %s\n", predicate_templates[0].c_str());
    // printf("predicate_templates 1: %s\n", predicate_templates[1].c_str());
    printf("predicate_name: %s\n", predicate_name.c_str());
    printf("size of predicate name map: %d\n", (int)predicate_templates.size());
    printf("fetching failed\n");
    predicate_template->Replace(PredicateTemplate());
    return false;
}

int PredicateTemplateHandler::GetPredicateIndex(string predicate_name)
{
    if (predicate_index_map.count(predicate_name) != 0)
    {
        return predicate_index_map.at(predicate_name);
    }
    printf("fetching blundered\n");
    return -1;
}
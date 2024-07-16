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

PredicateTemplateHandler::PredicateTemplateHandler() {}

PredicateTemplate PredicateTemplateHandler::GetPredicateTemplate(string predicate_name)
{
    return predicate_templates.at(predicate_name);
}

int PredicateTemplateHandler::GetPredicateIndex(string predicate_name)
{
    if (predicate_index_map.count(predicate_name) != 0)
    {
        return predicate_index_map.at(predicate_name);
    }
    return -1;
}
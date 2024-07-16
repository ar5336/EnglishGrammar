#ifndef PREDICATE_HANDLER_HPP
#define PREDICATE_HANDLER_HPP

#include <string>
#include <map>
#include <utility>
#include <set>

#include "predicate.hpp"
#include "string_operators.hpp"

enum KnowledgeType
{
    GIVEN,
    INFERRED,
};

enum ResponseType
{
    YES,
    NO,
};

const vector<string> predicate_strings
{
    "NONE",
    "IS_SUBSET_OF",
    "IS_INSTANCE_OF",
    "HAS_PROPERTY",
    "CAN_DO",
};

class PredicateHandler
{
private:
    map<string, string> inheritance_map;
    set<string> entity_set;
    map<string, vector<Predicate> > first_arg_to_predicate_map;
    // map<string, vector<Predicate>> entity_to_predicate_map;

    map<string, int> predicate_type_map;

    set<Predicate> given_predicates;
    set<Predicate> inferred_predicates;

    void UpdateInheritanceMap();

    vector<string> IdentifyAllParents(string entityName);

    string StringifyPredicate(Predicate predicate);
public:
    vector<pair<KnowledgeType, Predicate>> predicates;

    ResponseType DetermineResponse(Predicate queryPredicate);

    void tell(Predicate predicate);

    PredicateHandler();

    void InferPredicates();

    void AddPredicate(string type, vector<string> arguments);

    int PredIntFromString(string type);

    Predicate PredFromString(string input);

    PredicateTemplate GetPredicateTemplate(string predicate_string);
};

class PredicateTemplateReader
{
private:
    map<string, PredicateTemplate> predicate_templates;

public:
    PredicateTemplateReader(string predicates_file_name);

    PredicateTemplate GetPredicateTemplate(string predicate_string);
};

class PredicateTemplate
{
    string predicate;
    vector<string> parameter_names;
};

#endif
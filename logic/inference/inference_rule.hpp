#ifndef INFERENCE_RULE_HPP
#define INFERENCE_RULE_HPP

#include <vector>
#include "inference_predicate.hpp"
#include "../../grammar/predicate_rule_reader.hpp"
// #include "../../expression.hpp"

using namespace std;

class InferenceRule
{
private:
// the predicates that must be present for the rule to apply

    bool do_conditions_apply(Expression basis);
public:
    std::string name;
    
    vector<PredicateMatcher> condition_template;
    vector<PredicateCreator> conclusion_template;
    
    InferenceRule(const string& name,
                  const vector<PredicateMatcher>& condition_template,
                  const vector<PredicateCreator>& conclusion_template)
        : name(name), condition_template(condition_template), conclusion_template(conclusion_template) {}
};

class InferenceHandler
{
private:
    // a map from predicate type to predicate type
    //   by two argument names
    map<string, map<string, pair<string, string>>> existing_predicate_connections;

    // rule name -> source predicate type -> target predicate type -> (source argument, target argument)
    map<string, map<string, map<string, pair<string, string>>>> inference_rule_predicate_connections;

public:
    vector<InferenceRule> inference_rules;

    void add_inference_rule(const InferenceRule& rule);

    vector<Expression> apply_inference_rules(Expression knowledge_base);
};

#endif

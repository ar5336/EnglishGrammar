#ifndef INFERENCE_PREDICATE_HPP
#define INFERENCE_PREDICATE_HPP

#include <vector>
#include "../expression.hpp"

class InferencePredicate : public Predicate {
    vector<bool> are_params_discard;
public:
    InferencePredicate();

    InferencePredicate(int type_id, vector<string> arguments, PredicateTemplate predicate_template, vector<bool> are_params_discard);
};

 #endif
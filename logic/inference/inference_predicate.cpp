#include "inference_predicate.hpp"

InferencePredicate::InferencePredicate()
{
}

InferencePredicate::InferencePredicate(
    int type_id,
    vector<string> arguments,
    PredicateTemplate predicate_template,
    vector<bool> are_params_discard)
    : Predicate(type_id, arguments, predicate_template),
      are_params_discard(are_params_discard)
{
}

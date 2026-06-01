#ifndef INFERENCE_RULE_READER_HPP
#define INFERENCE_RULE_READER_HPP

#include <fstream>
#include <vector>

#include "../string_operators.hpp"
#include "frames.hpp"
#include "grammar.hpp"
// #include "../logic/inference/inference_rule.hpp"

using namespace std;

enum InferenceRuleReaderState{
    ReadingRuleName = 1,
    ReadingIf = 2,
    ReadingThen = 3,
    ReadingIntegrationTest = 4,
    Unknown = 5,
};

class InferenceRuleReader
{
private:
    PredicateHandler* predicate_handler;

    InferenceRuleReaderState state = InferenceRuleReaderState::Unknown;

    InferenceRuleReaderState categorize_line(const string line, const int current_indentation);

public:
    string rule_name;
    vector<PredicateMatcher> if_predicates;
    vector<PredicateCreator> then_predicates;
    vector<string> integration_tests; // not yet properly implemented as objects
    InferenceRuleReader(PredicateHandler* predicate_handler_ptr);

    bool is_rule_finished(const string line, const int current_indentation);

    void reset();

    // InferenceRule create_inference_rule();
};

#endif
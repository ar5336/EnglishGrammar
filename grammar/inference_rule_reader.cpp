#include "inference_rule_reader.hpp"

InferenceRuleReaderState InferenceRuleReader::categorize_line(const string line, const int current_indentation)
{
    string local_line = line;
    trim(local_line);
    if (line.size() == 0)
    {
        if (DEBUGGING)
            printf("error: line is empty\n");
        return InferenceRuleReaderState::Unknown;
    }
    
    // check if rule name is being read
    if (current_indentation == 1)
        return InferenceRuleReaderState::ReadingRuleName;

    // check if IF is being read
    if (current_indentation == 2 && equals(local_line, "IF"))
        return InferenceRuleReaderState::ReadingIf;

    if (current_indentation == 2 && equals(local_line, "THEN"))
        return InferenceRuleReaderState::ReadingThen;

    if (current_indentation == 2 && local_line.size() > 0 && local_line.at(0) == '#')
        return InferenceRuleReaderState::ReadingIntegrationTest;

    return InferenceRuleReaderState::Unknown;
}

InferenceRuleReader::InferenceRuleReader(PredicateHandler *predicate_handler_ptr)
{
    predicate_handler = predicate_handler_ptr;
}

pair<InferenceRuleReaderFinishType, string> InferenceRuleReader::is_rule_finished(const string line, const int current_indentation)
{
    // Parse the line and extract relevant information// FORMAT:
    // 	<RuleName>
    // 		IF
    // 			<predicate template> xN
    // 		THEN
    // 			<predicate template> xN
    //      # <integration test> xN

    auto category = categorize_line(line, current_indentation);

    if (if_predicates.size() != 0 && category == InferenceRuleReaderState::ReadingRuleName)
    {
        state = category;
        return make_pair(InferenceRuleReaderFinishType::NextRuleHandoff, line);
    }

    if (category == InferenceRuleReaderState::Unknown)
    {
        // if (DEBUGGING)
        //     printf("error: unrecognized line format: %s\n", line.c_str());
        // return false;
    }
    else
    {
        // if (state != category)
        //     return false;
        state = category;

        // don't process lines that indicate subsequent line's category
        if (state != InferenceRuleReaderState::ReadingRuleName
            && state != InferenceRuleReaderState::ReadingIntegrationTest)
            return make_pair(InferenceRuleReaderFinishType::NotFinished, "");
    }

    string local_line = line;
    trim(local_line);
    switch (state)
    {
    case InferenceRuleReaderState::ReadingRuleName:
        rule_name = local_line;
        // Handle rule name reading
        break;
    case InferenceRuleReaderState::ReadingIf:
        {
            vector<string> creation_tokens = split_character(local_line, " ");
            auto predicate_matcher = PredicateMatcher(predicate_handler, creation_tokens);
            if_predicates.emplace_back(predicate_matcher);
            break;
        }
    case InferenceRuleReaderState::ReadingThen:
        {
            vector<string> creation_tokens = split_character(local_line, " ");
            auto predicate_creator = PredicateCreator(predicate_handler, creation_tokens);
            then_predicates.emplace_back(predicate_creator);
            break;
        }
    case InferenceRuleReaderState::ReadingIntegrationTest:
        // Handle integration test reading
        integration_tests.push_back(local_line);
        break;
    default:
        break;
    }

    if ((current_indentation == 1 && state != InferenceRuleReaderState::ReadingRuleName)
        || (current_indentation == 0
            && (state == InferenceRuleReaderState::ReadingIntegrationTest
                || state == InferenceRuleReaderState::ReadingThen)))
        return make_pair(InferenceRuleReaderFinishType::Finished, "");

    return make_pair(InferenceRuleReaderFinishType::NotFinished, "");
}

void InferenceRuleReader::reset()
{
    state = InferenceRuleReaderState::ReadingRuleName;
    rule_name.clear();
    if_predicates.clear();
    then_predicates.clear();
    integration_tests.clear();
}

// InferenceRule InferenceRuleReader::create_inference_rule()
// {
//     return InferenceRule(rule_name, if_predicates, then_predicates);
// }

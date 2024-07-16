#include "test.hpp"


#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " #condition << " in " << __FILE__ \
                      << " line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while (false)

#define RUN_TEST(testFunc) \
    do { \
        std::cout << "Running test: " #testFunc << std::endl; \
        if (testFunc()) { \
            std::cout << "\033[1;32mPASS\033[0m" << std::endl; \
            passed++; \
        } else { \
            std::cout << "\033[1;31mFAIL\033[0m" << std::endl; \
            failed++; \
        } \
        total++; \
    } while (false)

int total = 0;
int passed = 0;
int failed = 0;

vector<Predicate> parse_utterance(string utterance)
{

}

bool parse_check(string utterance, vector<Predicate> expected_predicates)
{
    // auto result_redicates = parse_utterance(utterance);

    // if (expectedResults.size() != result_pre)
    // for (int i = 0; i < expec)
}


bool TestParsing() {
    // TEST_ASSERT(parse_utterance("dogs are cats") == 4);
    // TEST_ASSERT(1 + 1 == 2);
    return true;
}

bool testSubtraction() {
    // TEST_ASSERT(5 - 3 == 2);
    // TEST_ASSERT(10 - 5 == 5);
    return true;
}

int test_all() {
    // Grammar grammar = Grammar();
    // GrammarReader reader = GrammarReader(&grammar)
    // reader.read_grammar("grammar.txt");

    // grammar.binarize_grammar();

    // parser = Parser(grammar);

    // RUN_TEST(TestParsing);
    // RUN_TEST(testSubtraction);

    // std::cout << "\n\nTest Results:\n";
    // std::cout << "Total tests: " << total << std::endl;
    // std::cout << "\033[1;32mPassed: " << passed << "\033[0m" << std::endl;
    // std::cout << "\033[1;31mFailed: " << failed << "\033[0m" << std::endl;

    return 0;
}

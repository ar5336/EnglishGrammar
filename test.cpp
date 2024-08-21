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

string test_utterance = "";

Parser test_parser;

Grammar test_grammar = Grammar();

// Displayer displayer = Displayer("reader");

PredicateTemplateHandler test_predicate_template_handler = PredicateTemplateHandler();

PredicateHandler test_predicate_handler = PredicateHandler(&test_predicate_template_handler);

ConceptualSchema test_conceptual_schema = ConceptualSchema();

Mind test_mind = Mind(&test_predicate_handler, &test_conceptual_schema);

void setup_parse()
{
    GrammarReader test_reader = GrammarReader(&test_grammar, &test_predicate_handler, &test_predicate_template_handler);
	test_reader.read_grammar("grammar.txt");

	// translate the read frames into cnf frames
	test_grammar.binarize_grammar();

	test_parser = Parser(test_grammar);

	test_predicate_handler.predicate_template_handler = &test_predicate_template_handler;

	test_parser.predicate_handler = &test_predicate_handler;

	test_predicate_handler.init_stringification();
}

void run_parse(string utterance)
{
	test_parser.update_parse_grid(utterance);

    Frame frame = Frame();
    if (test_parser.try_get_top_interpretation(frame));

    Expression constructed_expression = frame.accumulated_expression;

    test_mind.tell(constructed_expression);

}

bool test_parse__event() {
    setup_parse();
	run_parse("a dog bit a man");

    TEST_ASSERT(test_mind.concrete_nouns.size() == 2);
    TEST_ASSERT(equals(test_mind.concrete_nouns.at(0).entity_type->noun, "dog"));
    TEST_ASSERT(equals(test_mind.concrete_nouns.at(1).entity_type->noun, "man"));

    return true;
}

bool test_parse__anaphora_event() {
    setup_parse();
	run_parse("a dog bit a man");
	run_parse("the dog that bit a man is ugly");

    TEST_ASSERT(test_mind.concrete_nouns.size() == 2);
    TEST_ASSERT(equals(test_mind.concrete_nouns.at(0).entity_type->noun, "dog"));
    TEST_ASSERT(equals(test_mind.concrete_nouns.at(1).entity_type->noun, "man"));

    TEST_ASSERT(equals(test_mind.concrete_nouns.at(1).entity_type->noun, "man"));

    return true;
}

int test_all() {
    RUN_TEST(test_parse__event);

    std::cout << "\n\nTest Results:\n";
    std::cout << "Total tests: " << total << std::endl;
    std::cout << "\033[1;32mPassed: " << passed << "\033[0m" << std::endl;
    std::cout << "\033[1;31mFailed: " << failed << "\033[0m" << std::endl;

    return 0;
}

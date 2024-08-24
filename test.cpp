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

ParserTester tester = ParserTester();

ParserTester::ParserTester()
    : test_predicate_handler(PredicateHandler(nullptr)),
        test_mind(Mind(nullptr, nullptr))
{
    test_utterance = "";

    test_parser = Parser();

    test_grammar = Grammar();

    // Displayer displayer = Displayer("reader");

    test_predicate_template_handler = PredicateTemplateHandler();

    test_predicate_handler = PredicateHandler(&test_predicate_template_handler);

    test_conceptual_schema = ConceptualSchema();

    test_mind = Mind(&test_predicate_handler, &test_conceptual_schema);
}

void test_handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}


void ParserTester::setup_parse()
{
    if(DEBUGGING)
        printf("setting up parse for test\n");
    test_predicate_template_handler = PredicateTemplateHandler();

    test_grammar = Grammar();

	signal(11, test_handler);   // install our handler


    GrammarReader test_reader = GrammarReader(&test_grammar, &test_predicate_handler, &test_predicate_template_handler);
	test_reader.read_grammar("grammar.txt");

 	// translate the read frames into cnf frames
	test_grammar.binarize_grammar();

    test_predicate_handler = PredicateHandler(&test_predicate_template_handler);

    test_conceptual_schema = ConceptualSchema();

	test_parser = Parser(test_grammar);
	test_predicate_handler.predicate_template_handler = &test_predicate_template_handler;
	test_parser.predicate_handler = &test_predicate_handler;


	test_predicate_handler.init_stringification();

    test_mind = Mind(&test_predicate_handler, &test_conceptual_schema);

}

void ParserTester::run_parse(string utterance)
{
	test_parser.update_parse_grid(utterance);

    Frame frame = Frame();
    if (test_parser.try_get_top_interpretation(frame))
    {
        if (DEBUGGING)
        {
            printf("recieved expression from parser\n");
        }
        Expression constructed_expression = frame.accumulated_expression;
        test_mind.tell(constructed_expression);
    }
}

bool test_parse__event() {
    tester.setup_parse();
	tester.run_parse("a dog bit a man");

    TEST_ASSERT(tester.test_mind.concrete_nouns.size() == 2);
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(0).entity_type->noun, "dog"));
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(1).entity_type->noun, "man"));

    return true;
}

bool test_parse__anaphora_event() {
    tester.setup_parse();
	tester.run_parse("a dog bit a man");
	tester.run_parse("the dog that bit a man is ugly");

    TEST_ASSERT(tester.test_mind.concrete_nouns.size() == 2);
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(0).entity_type->noun, "dog"));
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(1).entity_type->noun, "man"));

    TEST_ASSERT(tester.test_mind.concrete_nouns.at(0).properties.count("ugly") == 1);

    return true;
}

bool test_parse__event_anaphora_create_event() {
    tester.setup_parse();
	tester.run_parse("a dog bit a man");
	tester.run_parse("the dog that bit a man is ugly");
	tester.run_parse("the dog that bit a man ate some grass");

    TEST_ASSERT(tester.test_mind.concrete_nouns.size() == 3);
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(0).entity_type->noun, "dog"));
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(1).entity_type->noun, "man"));
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(2).entity_type->noun, "grass"));

    TEST_ASSERT(tester.test_mind.timeline.actions.size() == 2);

    Event dbm = tester.test_mind.timeline.actions.at(0);
    TEST_ASSERT(equals(dbm.actor_noun_class, "dog"));
    TEST_ASSERT(equals(dbm.subject_noun_class, "man"));

    Event dag = tester.test_mind.timeline.actions.at(1);
    TEST_ASSERT(equals(dag.actor_noun_class, "dog"));
    TEST_ASSERT(equals(dag.subject_noun_class, "grass"));

    TEST_ASSERT(tester.test_mind.concrete_nouns.at(0).properties.count("ugly") == 1);

    return true;
}

int test_all() {
    RUN_TEST(test_parse__event);
    RUN_TEST(test_parse__anaphora_event);
    RUN_TEST(test_parse__event_anaphora_create_event);

    std::cout << "\n\nTest Results:\n";
    std::cout << "Total tests: " << total << std::endl;
    std::cout << "\033[1;32mPassed: " << passed << "\033[0m" << std::endl;
    std::cout << "\033[1;31mFailed: " << failed << "\033[0m" << std::endl;

    return 0;
}

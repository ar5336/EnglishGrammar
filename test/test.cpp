#include "test.hpp"


#define TEST_ASSERT(condition) \
    if (!(condition)) { \
        std::cerr << "\nAssertion failed: " #condition << " in " << __FILE__ \
                    << " line " << __LINE__ << std::endl; \
        return false; \
    } else { \
        std::cout << "\033[1;32m█\033[0m";\
    } \


#define RUN_TEST(testFunc) \
    std::cout << "Running test: " #testFunc << std::endl; \
    if (testFunc()) { \
        std::cout << "\033[1;32mPASS\033[0m" << std::endl; \
        passed++; \
    } else { \
        std::cout << "\033[1;31mFAIL\033[0m" << std::endl; \
        failed++; \
    } \
    total++; \

int total = 0;
int passed = 0;
int failed = 0;

ParserTester tester = ParserTester();

ParserTester::ParserTester()
    : test_predicate_handler(PredicateHandler(nullptr)),
        test_mind(Mind(nullptr, nullptr))
{
    test_utterance = "";

    test_variable_namer = VariableNamer();

    test_parser = Parser();

    test_grammar = Grammar();

    // Displayer displayer = Displayer("reader");

    test_predicate_template_handler = PredicateTemplateHandler();

    test_predicate_handler = PredicateHandler(&test_predicate_template_handler);

    test_conceptual_schema = ConceptualSchema();

    test_mind = Mind(&test_predicate_handler, &test_conceptual_schema);
}

void ParserTester::setup_parse()
{
    if(DEBUGGING)
        printf("setting up parse for test\n");

    test_predicate_template_handler = PredicateTemplateHandler();

    test_variable_namer = VariableNamer();

    test_predicate_handler = PredicateHandler(&test_predicate_template_handler);
    test_grammar = Grammar(&test_predicate_handler, &test_variable_namer);

	// signal(11, handler);   // install our handler

    GrammarReader test_reader = GrammarReader(&test_grammar, &test_predicate_handler, &test_predicate_template_handler);
	test_reader.read_grammar("grammar.langdef");

 	// translate the read frames into cnf frames
	test_grammar.binarize_grammar();


    test_conceptual_schema = ConceptualSchema();

	test_parser = Parser(test_grammar, &test_variable_namer);
	test_predicate_handler.predicate_template_handler = &test_predicate_template_handler;
	test_parser.predicate_handler = &test_predicate_handler;


	test_predicate_handler.init_stringification();

    test_mind = Mind(&test_predicate_handler, &test_conceptual_schema);

    tell_mind("horses are mammals");
    tell_mind("horses are mammals");
    tell_mind("dogs are mammals");
    tell_mind("foxes are mammals");
    tell_mind("fishes are animals");
}

void ParserTester::tell_mind(string utterance)
{
	test_parser.update_parse_grid(utterance);

    Frame frame = Frame();
    if (test_parser.try_get_top_frame(frame))
    {
        if (DEBUGGING)
        {
            printf("recieved expression from parser\n");
        }
        Expression constructed_expression = frame.accumulated_expression;
        test_mind.tell(constructed_expression);
    }
}

string ParserTester::ask_mind(string utterance)
{
    test_parser.update_parse_grid(utterance);

    Frame frame = Frame();
    if (test_parser.try_get_top_frame(frame))
    {
        if (DEBUGGING)
        {
            printf("recieved expression from parser\n");
        }
        Expression constructed_expression = frame.accumulated_expression;
        return test_mind.ask(constructed_expression);
    }
    return "dunno";
}

bool test_parse__event() {
    tester.setup_parse();
	tester.tell_mind("a dog bit a man");

    TEST_ASSERT(tester.test_mind.concrete_nouns.size() == 2);
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(0).entity_type->noun, "dog"));
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(1).entity_type->noun, "man"));

    TEST_ASSERT(equals(tester.ask_mind("did a dog bite a man"), "yes, it did happen"));
    TEST_ASSERT(equals(tester.ask_mind("did a dog bite a fish"), "no, it did not happen"));

    return true;
}

bool test_parse__anaphora_event() {
    tester.setup_parse();
	tester.tell_mind("a dog bit a man");
	tester.tell_mind("the dog that bit a man is ugly");

    TEST_ASSERT(tester.test_mind.concrete_nouns.size() == 2);
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(0).entity_type->noun, "dog"));
    TEST_ASSERT(equals(tester.test_mind.concrete_nouns.at(1).entity_type->noun, "man"));

    TEST_ASSERT(tester.test_mind.concrete_nouns.at(0).properties.count("ugly") == 1);

    return true;
}

bool test_parse__event_anaphora_create_event() {
    tester.setup_parse();
	tester.tell_mind("a dog bit a man");
	tester.tell_mind("the dog that bit a man is ugly");
	tester.tell_mind("the dog that bit a man ate some grass");

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

bool test_parse__passive_voice() {
    tester.setup_parse();
	tester.tell_mind("a man got bit by a horse");

    Event dbm = tester.test_mind.timeline.actions.at(0);
    TEST_ASSERT(equals(dbm.actor_noun_class, "horse"));
    TEST_ASSERT(equals(dbm.subject_noun_class, "man"));

    TEST_ASSERT(equals(tester.ask_mind("a man get bit by a horse"), "yes, it did happen"));

    return true;
}

bool test_parse__event_rephrasings() {
    tester.setup_parse();
	tester.tell_mind("a dog bit a man");

    TEST_ASSERT(equals(tester.ask_mind("did a dog bite a man"), "yes, it did happen"));

    TEST_ASSERT(equals(tester.ask_mind("did a man get bit by a dog"), "yes, it did happen"));
    tester.tell_mind("the man that the dog bit is ugly");
    tester.tell_mind("the man the dog bit is tall");

    TEST_ASSERT(equals(tester.ask_mind("is the man the dog bit ugly"), "yes, it does have that property"));
    TEST_ASSERT(equals(tester.ask_mind("is the man the dog bit tall"), "yes, it does have that property"));
    return true;
}

bool test_parse__properties() {
    tester.setup_parse();
	tester.tell_mind("a quick brown fox jumps over the lazy dog");
    // TODO - also processing + tests for predicates

    TEST_ASSERT(equals(tester.ask_mind("did a fox jump"), "yes, it did happen"));
    TEST_ASSERT(equals(tester.ask_mind("was the fox that jumped quick"), "yes, it does have that property"));
    TEST_ASSERT(equals(tester.ask_mind("was the fox that jumped brown"), "yes, it does have that property"));
    TEST_ASSERT(equals(tester.ask_mind("was the fox that jumped ugly"), "no, it does not have the property 'ugly'"));

	tester.tell_mind("an ugly fish bit a beautiful brown dog");
    TEST_ASSERT(equals(tester.ask_mind("is the fish that bit the dog ugly"), "yes, it does have that property"));
    TEST_ASSERT(equals(tester.ask_mind("is the dog that got bit by the fish beautiful"), "yes, it does have that property"));
    TEST_ASSERT(equals(tester.ask_mind("did a fish bite a dog"), "yes, it did happen"));

    return true;
}

// bool test_parse__nested_anaphoras() {
//     // did a dog bite a horse that ate some grass isn't working as expected
// }

bool run_integration_test()
{
    tester.setup_parse();

    fstream new_file;
    new_file.open("grammar.langdef", ios::in); // open a file to perform read operation using file object
    if (!new_file.is_open()) {
        throw runtime_error("Failed to open grammar file.");
        return false;
    }

    bool reading_frames = false;
    string current_line = "";

    int current_definition_line = 0;
    int current_reading_line = 0;
    if (new_file.is_open())
    {
        while (getline(new_file, current_line))
        {
            current_reading_line++;
            // printf("line %d: %s\n", current_reading_line, current_line.c_str());

            if (current_line.size() == 0)
            {
                continue;
            }
            
            int initial_spaces = count_initial_spaces(current_line);

            trim(current_line);
            bool has_hashtag = current_line.at(0) == '#';
            if (has_hashtag)
                current_line = current_line.substr(1, current_line.size());
            trim(current_line);

            bool has_bang = current_line.at(0) == '!';
            if (has_bang)
                current_line = current_line.substr(1, current_line.size());
            trim(current_line);

            bool has_left_bracket = find_in_string(current_line, "<");
            bool has_right_bracket = find_in_string(current_line, ">");


            if (initial_spaces == 7) // syntax definition
            {
                if (!has_hashtag)
                    current_definition_line = current_reading_line;
            }

            if (initial_spaces == 11) // check for correct indentation
            {


                if (!has_hashtag)
                {
                    continue;
                }

                if (DEBUGGING)
                {
                    printf("scanning line %s\n", current_line.c_str());
                }

                vector<string> split_tokens = split_spaces(current_line);
                trim(current_line);

                Frame top_frame = Frame();

                int start_bracket_index = 0;
                int end_bracket_index = split_tokens.size()-1;

                vector<string> modified_tokens = vector<string>();
                for (int i = 0; i < split_tokens.size(); i++)
                {
                    string token = split_tokens.at(i);
                    if (find_in_string(token, "<"))
                    {
                        token = remove_char_from_string(token, '<');

                        start_bracket_index = i;
                    }
                    else if (find_in_string(token, ">"))
                    {
                        token = remove_char_from_string(token, '>');
                        end_bracket_index = i;
                    }
                    modified_tokens.push_back(token);
                }

                // # the dog <that | ate> the horse
                // TODO - make the crossbar actiually matter

                int frame_boundary_pivot = -1;

                vector<string> new_modified_tokens;
                for (int i = 0; i < modified_tokens.size(); i++)
                {
                    string token = modified_tokens.at(i);
                    if (token.size() == 1 && token.at(0) == '|')
                    {
                        // remove the token
                        end_bracket_index--;
                        frame_boundary_pivot = i;
                    } else
                    {
                        new_modified_tokens.push_back(token);
                    }
                }

                string collated_line = "";
                for (string token : new_modified_tokens)
                {
                    collated_line += (token + " ");
                }
                trim(collated_line);


                tester.tell_mind(collated_line);

                bool does_have_interpretation = false;
                Frame matching_frame;

                int def_line = -1;

                auto interp_frames = tester.test_parser.get_interpret_frames(start_bracket_index, end_bracket_index - start_bracket_index);
                for(auto interp_frame : interp_frames)
                {
                    int cur_def_line = interp_frame.definition_line;

                    if (cur_def_line == current_definition_line)
                    {
                        matching_frame = interp_frame;
                        does_have_interpretation = true;
                        def_line = cur_def_line;
                    }
                }

                // if (DEBUGGING)
                    printf("'%s' | result: %d : expected: %d\n",current_line.c_str(), def_line,  current_definition_line);

                // // printf("stringified parser: %s\n", tester.test_parser.stringify().c_str());

                // // printf("frame index: %d\n", tester.test_parser.parse_grid[0][0][0].definition_line);

                TEST_ASSERT(has_bang xor does_have_interpretation);
            }
        }
    }

    return true;;
}


int test_all() {
    printf("running unit tests\n");

    RUN_TEST(test_parse__event);
    RUN_TEST(test_parse__anaphora_event);
    RUN_TEST(test_parse__event_anaphora_create_event);
    RUN_TEST(test_parse__passive_voice);
    RUN_TEST(test_parse__event_rephrasings);
    RUN_TEST(test_parse__properties);

    printf("running integration tests\n");

    RUN_TEST(run_integration_test);

    std::cout << "\n\nTest Results:\n";
    std::cout << "Total tests: " << total << std::endl;
    std::cout << "\033[1;32mPassed: " << passed << "\033[0m" << std::endl;
    if (failed != 0)
        std::cout << "\033[1;31mFailed: " << failed << "\033[0m" << std::endl;

    return 0;
}

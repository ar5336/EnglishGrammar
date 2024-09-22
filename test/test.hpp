#ifndef TEST
#define TEST

#include <execinfo.h>
#include <iostream>
#include <signal.h>

#include "../grammar/grammar_reader.hpp"

#include "../logic/interpretation.hpp"
#include "../logic/mind.hpp"

#include "../global.hpp"

class ParserTester
{
private:

    string test_utterance;

    VariableNamer test_variable_namer;

    Grammar test_grammar;

    // Displayer displayer = Displayer("reader");

    PredicateTemplateHandler test_predicate_template_handler;


    ConceptualSchema test_conceptual_schema;

public:
    PredicateHandler test_predicate_handler;

    Parser test_parser;

    Mind test_mind;

    void tell_mind(string utterance);

    string ask_mind(string utterance);

    void setup_parse();

    ParserTester();


};

int test_all();

#endif

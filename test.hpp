#ifndef TEST
#define TEST

#include <execinfo.h>
#include <iostream>
#include <signal.h>

#include "grammar_reader.hpp"
#include "interpretation.hpp"
#include "mind.hpp"
#include "global.hpp"

class ParserTester
{
private:

    string test_utterance;

    Parser test_parser;

    Grammar test_grammar;

    // Displayer displayer = Displayer("reader");

    PredicateTemplateHandler test_predicate_template_handler;

    PredicateHandler test_predicate_handler;

    ConceptualSchema test_conceptual_schema;

public:

    Mind test_mind;

    void tell_mind(string utterance);

    string ask_mind(string utterance);

    void setup_parse();

    ParserTester();


};

int test_all();

#endif

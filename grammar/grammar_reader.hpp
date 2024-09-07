#ifndef GRAMMAR_READER_HPP
#define GRAMMAR_READER_HPP

#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "../string_operators.hpp"
#include "frames.hpp"
#include "grammar.hpp"

using namespace std;

enum GrammarReaderState{
	ReadingWords = 1,
	ReadingSyntax = 2,
	ReadingFeatureGroups = 3,
	ReadingPredicateTemplates = 4,
};

class GrammarReader
{
private:
    Grammar* grammar;
	PredicateHandler* predicate_handler;
	PredicateTemplateHandler* predicate_template_handler;

	GrammarReaderState state;
	string current_line;
	int current_line_index;

	int previous_indentation = 0;

	vector<PatternNecessity> term_forms;
	vector<string> term_form_names;

	vector<string> type_heirarchy;

	string pattern_name;
	string pattern_nickname;

	string first_token;
	vector<string> split_tokens;

	void add_term_forms(
		vector<PatternNecessity>& term_form_types,
		vector<string>& term_form_names);

	void read_syntax_entry();

	void read_feature_group_entry();

	void read_word_entry();

	void read_predicate_template_entry();

public:
	GrammarReader(Grammar *grammar_ptr, PredicateHandler *predicate_handler_ptr, PredicateTemplateHandler* predicate_template_handler);

	void read_grammar(string fileName);
};

#endif
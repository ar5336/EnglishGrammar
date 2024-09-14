#include <iostream>
#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include <vector>
#include <map>
#include <fstream>
#include <set>

#include <unistd.h>

#include "grammar/grammar_reader.hpp"
#include "grammar/grammar.hpp"

#include "logic/interpretation.hpp"
#include "logic/mind.hpp"

#include "test/test.hpp"

#include "displayer.hpp"
#include "global.hpp"

using namespace std;
using namespace cv;

string initial_utterance = "";

string current_utterance = "";

Parser parser;

Grammar grammar = Grammar();

Displayer displayer = Displayer("reader");

PredicateTemplateHandler predicate_template_handler = PredicateTemplateHandler();

PredicateHandler predicate_handler = PredicateHandler(&predicate_template_handler);

ConceptualSchema conceptual_schema = ConceptualSchema();

Mind mind = Mind(&predicate_handler, &conceptual_schema);

bool is_shift_pressed = false;

vector<string> known_facts = {
	"dogs are mammals",
	"people are mammals",
	"mammals are animals",
	"ravens are birds",
	"birds are animals",
	"horses are mammals",
	"foxes are mammals",
	"birds can fly",
	"animals can breathe",
	"animals can bite"};
	// "a raven flew",
	// "the raven that flew bit a horse"};
	// "a dog bit a man"};

void parse_utterance(string utterance)
{
	parser.update_parse_grid(utterance);
	
	Frame interp_frame;
	if (!parser.try_get_top_frame(interp_frame))
	{
		printf("failed to interpret given utterance \"%s\"\n", utterance.c_str());
		return;
	}

	auto interp_handler = InterpretationHandler(&parser, interp_frame);

	Expression expression;
	if(!interp_handler.try_construct_expression(expression))
	{
		printf("failed to construct expression for given utternace \"%s\"\n", utterance.c_str());
		return;
	}

	mind.tell(expression);

}

void parse_known_facts()
{
	for (string fact : known_facts)
	{
		parse_utterance(fact);
		displayer.display();
	}
	parser.update_parse_grid(initial_utterance);
}

void mouse_callback_function(int event, int x, int y, int flags, void *userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		bool highlight_found = false;
		for (int row = 0; row < parser.parse_grid.size() && !highlight_found; row++)
		{
			for (int col = 0; col < parser.parse_grid.at(row).size() && !highlight_found; col++)
			{
				pair<Point, Point> cell_bounds = displayer.get_cell_bounds(row, col);
				if (displayer.is_in_bounds(Point(x, y), cell_bounds))
				{
					displayer.highlighted_cell_position = pair<int, int>(row, col);
					highlight_found = true;
					displayer.is_highlighted = true;
					break;
				}
			}
		}

		if (!highlight_found)
		{
			displayer.is_highlighted = false;
		}
		displayer.display();
	}
	else if (event == EVENT_RBUTTONDOWN) { }
	else if (event == EVENT_MBUTTONDOWN) { }
	else if (event == EVENT_MOUSEMOVE) { }
	else if (event == EVENT_MOUSEWHEEL) { }
}

bool check_keypress(char cr)
{
	string current_utterance = parser.current_utterance;
	displayer.is_highlighted = false;

	if (cr == -31) // shift
	{
		is_shift_pressed = true;
	}

	if (!is_shift_pressed){
		if (cr == 81) // left
		{
		}
		if (cr == 82 ) // up
		{
			displayer.scroll -= 10;
		}
		if (cr == 83) // right
		{
		}
		if (cr == 84)
		{
			displayer.scroll += 10;
		}
	}
	

	if (cr == 27)
	{
		return true; // break
	}
	else
	{
		displayer.response_string = "";
		is_shift_pressed = false;
		if (cr >= 97 && cr <= 122 || cr == 32)
		{
			string new_utterance = current_utterance += cr;
			parser.update_parse_grid(new_utterance);

		}

		if (cr == 8)
		{ // backspace{
			int utterance_size = current_utterance.size();
			string new_utterance = current_utterance;
			if (utterance_size >= 1)
				new_utterance = parser.current_utterance.substr(0, utterance_size - 1);
			parser.update_parse_grid(new_utterance);
		}
		if (cr == 13)
		{ // enter
			// interpret the sentence

			auto base_frame = Frame();
			if (parser.try_get_top_frame(base_frame)){
				auto interp_handler = InterpretationHandler(&parser, base_frame);

				auto expression = Expression();
				if (interp_handler.try_construct_expression(expression))
				{
					if (equals(base_frame.frame_name, "Question"))
					{
						auto response = mind.ask(expression);

						displayer.response_string = response;
					} else
					{
						mind.tell(expression);
					}
				} else
				{
					printf("failed to construct expression\n");
				}
				displayer.display();
			}
		}
		if (cr == '\'')
		{
			parser.current_utterance += '\'';
		}

		return false;
	}
}

int main(int argc, char **argv)
{
	int arg_index = 1;
	set<string> debug_alternatives = set<string> {"--debug", "-d"};
	if (argc > 1 && (debug_alternatives.count((string)argv[arg_index]) != 0))
	{
		DEBUGGING = true;
		arg_index++;
	}

	set<string> test_alternatives = set<string> {"--test", "-t"};
	if (argc > arg_index && (test_alternatives.count((string)argv[arg_index]) != 0))
	{
		test_all();
		return 1;
	}

	// signal(11, handler);   // install our handler

	// read the grammar
	GrammarReader reader = GrammarReader(&grammar, &predicate_handler, &predicate_template_handler);
	reader.read_grammar("grammar.langdef");

	// translate the read frames into cnf frames
	grammar.binarize_grammar();

	parser = Parser(grammar);

	predicate_handler.predicate_template_handler = &predicate_template_handler;

	parser.predicate_handler = &predicate_handler;

	predicate_handler.init_stringification();

	displayer.init(&parser, &mind, &predicate_handler, &conceptual_schema);
	displayer.display();
    setMouseCallback(displayer.screen_name, mouse_callback_function, NULL);

	parser.update_parse_grid(current_utterance);

	// displayer.display();
	parse_known_facts();

	while (1)
	{
		displayer.display();
		// displayer.drift();
		if (check_keypress((char)waitKey(0)))
		{
			break;
		}

		// match word frames against the text
		displayer.display();
	}
	destroyAllWindows();

	return 0;
}
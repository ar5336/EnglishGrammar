#include <iostream>
#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include </usr/local/include/opencv4/opencv2/tracking.hpp>
#include <vector>
#include <map>
#include <fstream>
#include <set>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "grammar_reader.hpp"
#include "grammar.hpp"
#include "displayer.hpp"
#include "interpretation.hpp"
#include "mind.hpp"
#include "test.hpp"
#include "global.hpp"

using namespace std;
using namespace cv;

string current_utterance = "the ugly dog jumps over the hors";

Parser parser;

Grammar grammar = Grammar();

Displayer displayer = Displayer("reader");

PredicateTemplateHandler predicate_template_handler = PredicateTemplateHandler();

PredicateHandler predicate_handler = PredicateHandler(&predicate_template_handler);

ConceptualSchema conceptual_schema = ConceptualSchema();

Mind mind = Mind(&predicate_handler, &conceptual_schema);

bool is_shift_pressed = false;

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
					parser.highlighted_cell_position = pair<int, int>(row, col);
					highlight_found = true;
					parser.is_highlighted = true;
					break;
				}
			}
		}

		if (!highlight_found)
		{
			parser.is_highlighted = false;
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
	parser.is_highlighted = false;

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
			if (parser.try_get_top_interpretation(base_frame)
				// && (equals(base_frame.frame_name, "Sentence")
				// || equals(base_frame.frame_name, "Question"))
				){
				auto interp_handler = InterpretationHandler(&parser, base_frame);

				auto expression = Expression();
				if (interp_handler.try_construct_expression(expression))
				{
					// printf("expression string: \n\n%s\n", expression.stringify().c_str());

					// if (equals(base_frame.frame_name, "Sentence"))
					// {
					// }
					if (equals(base_frame.frame_name, "Question"))
					{
						auto response = mind.ask(expression);

						displayer.response_string = response;
					} else {
						mind.tell(expression);
					}
				} else {
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

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char **argv)
{
	if (argc > 1 && ((string)argv[1] == "--test"))
	{
		test_all();
		return 1;
	}

	set<string> debug_alternatives = {"--debug", "-d"};
	if (argc > 1 && (debug_alternatives.count((string)argv[1]) != 0))
	{
		DEBUGGING = true;
	}

	signal(11, handler);   // install our handler

	// read the grammar
	GrammarReader reader = GrammarReader(&grammar, &predicate_handler, &predicate_template_handler);
	reader.read_grammar("grammar.txt");

	// translate the read frames into cnf frames
	grammar.binarize_grammar();

	parser = Parser(grammar);

	predicate_handler.predicate_template_handler = &predicate_template_handler;

	parser.predicate_handler = &predicate_handler;

	predicate_handler.init_stringification();

	displayer.init(&parser, &mind, &predicate_handler);
	displayer.display();
    setMouseCallback(displayer.screen_name, mouse_callback_function, NULL);

	parser.update_parse_grid(current_utterance);

	// displayer.display();

	while (1)
	{
		displayer.display();
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
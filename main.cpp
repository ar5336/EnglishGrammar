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
#include "frames.hpp"
#include "grammar.hpp"
#include "string_operators.hpp"
#include "parser.hpp"
#include "displayer.hpp"
#include "interpretation.hpp"
#include "predicate_handler.hpp"
#include "test.hpp"

using namespace std;
using namespace cv;

string current_utterance = "the quick brown fox";

Parser parser;

Grammar grammar = Grammar();

Displayer displayer = Displayer("reader");

PredicateTemplateHandler predicate_template_handler = PredicateTemplateHandler();

PredicateHandler predicate_handler = PredicateHandler(&predicate_template_handler);

bool is_shift_pressed = false;

void mouse_callback_function(int event, int x, int y, int flags, void *userdata)
{
	// still doesn't work
	if (event == EVENT_LBUTTONDOWN)
	{
		cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
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
				&& (equals(base_frame.frame_name, "Sentence")
				|| equals(base_frame.frame_name, "Question"))){
				auto interp_handler = InterpretationHandler(&parser, base_frame);

				auto expression = Expression();
				if (interp_handler.TryConstructExpression(expression))
				{
					predicate_handler.tell(expression);
					predicate_handler.InferExpressions();
					// if (predicate.speech_act == SpeechActs::QUESTION) {
					// 	auto response = predicate_handler.DetermineResponse(predicate);
					// 	if (response == ResponseType::YES) {
					// 		displayer.response_string = "Yes";
					// 	}
					// 	if (response == ResponseType::NO) {

					// 		displayer.response_string = "No";
					// 	}
					// } else {
					// 	predicate_handler.tell(predicate);
					// 	predicate_handler.InferPredicates();
					// }
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

	signal(11, handler);   // install our handler

	// read the grammar
	GrammarReader reader = GrammarReader(&grammar, &predicate_handler, &predicate_template_handler);
	reader.read_grammar("testgrammar.txt");

	// translate the read frames into cnf frames
	grammar.binarize_grammar();

	parser = Parser(grammar);

	displayer.init(&parser, &predicate_handler);
    setMouseCallback(displayer.screen_name, mouse_callback_function, NULL);

	parser.update_parse_grid(current_utterance);

	displayer.display();

	while (1)
	{
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
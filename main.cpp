#include <iostream>
#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include </usr/local/include/opencv4/opencv2/tracking.hpp>
#include <vector>
#include <map>
#include <fstream>
#include <set>

#include "grammar_reader.hpp"
#include "frames.hpp"
#include "grammar.hpp"
#include "string_operators.hpp"
#include "parser.hpp"
#include "displayer.hpp"

using namespace std;
using namespace cv;

string current_utterance = "";

Parser parser;

Grammar grammar = Grammar();

Displayer displayer = Displayer("reader");

void mouse_callback_function(int event, int x, int y, int flags, void *userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		// cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
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
}

bool check_keypress(char cr)
{
	string current_utterance = parser.current_utterance;
	parser.is_highlighted = false;
	if (cr == 27)
	{
		return true; // break
	}
	else
	{
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
			// update the cyk grid with the latest utterance
			parser.update_parse_grid(current_utterance);
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
	// Mat image;
	// read the grammar
	GrammarReader reader = GrammarReader(&grammar);
	reader.read_grammar("grammar.txt");

	// translate the read frames into cnf frames
	grammar.binarize_grammar();

	parser = Parser(grammar);

	displayer.init(&parser, mouse_callback_function);

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
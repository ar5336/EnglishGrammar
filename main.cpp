#include <iostream>
#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include </usr/local/include/opencv4/opencv2/tracking.hpp>
#include </usr/local/include/opencv4/opencv2/videoio/videoio_c.h>
#include <vector>
#include <map>
#include <fstream>
#include <set>

#include "grammar_reader.hpp"
#include "frames.hpp"
#include "grammar.hpp"
#include "string_operators.hpp"
#include "parser.hpp"

using namespace std;
using namespace cv;

string current_utterance = "";

Parser parser;

Grammar grammar = Grammar();

pair<Point, Point> get_cell_bounds(Point start_grid_corner, int row, int col)
{
	int cell_width = 80;
	int cell_height = 20;

	Point top_left = start_grid_corner + Point(col * cell_width + (row * cell_width / 2), -(row * cell_height));
	Point bottom_right = top_left + Point(cell_width, cell_height);

	return pair<Point, Point>(top_left, bottom_right);
}

bool is_in_bounds(Point point, pair<Point, Point> bounds)
{
	Point top_left = bounds.first;
	Point bottom_right = bounds.second;

	return point.x >= top_left.x && point.x <= bottom_right.x && point.y >= top_left.y && point.y <= bottom_right.y;
}


bool check_keypress(char cr)
{
	parser.is_highlighted = false;
	if (cr == 27)
	{
		return true; // break
	}
	else
	{
		// printf("character: %d\n", cr);
		if (cr >= 97 && cr <= 122 || cr == 32)
		{
			current_utterance += cr;
			parser.update_parse_grid(current_utterance);

			// display();
		}

		if (cr == 8)
		{ // backspace{
			int utterance_size = current_utterance.size();
			if (utterance_size >= 1)
				current_utterance = current_utterance.substr(0, utterance_size - 1);
			parser.update_parse_grid(current_utterance);
		}
		if (cr == 13)
		{ // enter
			// update the cyk grid with the latest utterance
			parser.update_parse_grid(current_utterance);
		}
		if (cr == '\'')
		{
			current_utterance += '\'';
		}

		return false;
	}
}


void display_text(Mat img, Point pos, string text, Scalar color, float font_scale = 1.0)
{
	putText(img,  // target image
			text, // text
			pos,
			cv::FONT_HERSHEY_TRIPLEX,
			font_scale,
			color, // font color
			1);
}

Size measure_text(string text, float font_scale = 1.0)
{
	int was_found;
	return getTextSize(text,
					   cv::FONT_HERSHEY_TRIPLEX,
					   1.0,
					   2,
					   &was_found);
}

string screen_name = "reader";
Mat img(512, 1024, CV_8UC3, cv::Scalar(0));
Point start_text_corner = cv::Point(10, img.rows * 3 / 4); // top-left position
Point start_grid_corner = start_text_corner + Point(0, -60);

Scalar HIGHLIGHTER_YELLOW = CV_RGB(50, 25, 0);

void display()
{
	vector<string> split_tokens;
	vector<Frame> word_frames;
	boost::split(split_tokens, current_utterance, boost::is_any_of(" "), boost::token_compress_on);

	img.setTo(Scalar(0)); // clear screen

	int token_count = split_tokens.size();

	vector<bool> is_word_highlighted;

	if (!parser.parse_grid.empty() && token_count != 0)
	{
		// diplay and initialize parse grid

		//  r3  X
		//  r2  X  X
		//  r1  X  X  X
		//  r0  X  X  X  X
		//      c0 c1 c2 c3

		int cell_width = 80;
		int cell_height = 20;

		for (int row = 0; row < parser.parse_grid.size(); row++)
		{
			for (int col = 0; col < parser.parse_grid.at(row).size(); col++)
			{

				pair<Point, Point> cell_bounds = get_cell_bounds(start_grid_corner, row, col);

				Point top_left = cell_bounds.first;
				Point bottom_right = cell_bounds.second;

				bool is_covered_by_highlight;
				bool is_this_cell_selected;
				if (parser.is_highlighted)
				{
					int highlight_row = parser.highlighted_cell_position.first;
					int highlight_col = parser.highlighted_cell_position.second;
					int d_row = highlight_row - row;
					int d_col = col - highlight_col;

					is_covered_by_highlight = (row <= highlight_row && col >= highlight_col && d_col <= d_row);
				}
				else
				{
					is_covered_by_highlight = false;
				}

				if (row == 0)
					is_word_highlighted.push_back(is_covered_by_highlight);

				// check if this cell is highlighted
				if (is_covered_by_highlight)
				{

					rectangle(img, top_left, bottom_right, HIGHLIGHTER_YELLOW, cv::FILLED);
				}
				else
				{
					rectangle(img, top_left, bottom_right, CV_RGB(0, 0, 0), cv::FILLED);
				}

				// draw rectangle
				rectangle(img, top_left, bottom_right, CV_RGB(255, 255, 255));

				vector<Frame> frames_in_cell = parser.parse_grid.at(row).at(col);

				float cell_font_scale = .5f;

				Point bottom_left = top_left + Point(3, cell_height - 3);
				Point ticker_cell_text = bottom_left;
				for (int frame_index = 0; frame_index < frames_in_cell.size(); frame_index++)
				{
					Frame frame = frames_in_cell.at(frame_index);
					// display word
					string cell_text;
					if (row == 0)
					{
						cell_text = frame.get_part_of_speech();
						display_text(img, ticker_cell_text, cell_text, CV_RGB(100, 100, 200), cell_font_scale);
					}
					else
					{
						// display frame
						cell_text = frame.frame_nickname;
						display_text(img, ticker_cell_text, cell_text, CV_RGB(100, 200, 100), cell_font_scale);
					}
					ticker_cell_text += Point(measure_text(cell_text, cell_font_scale).width, 0);
				}
			}
		}
	}

	// display the text
	Point ticker_text_corner = start_text_corner;
	for (int token_index = 0; token_index < split_tokens.size(); token_index++)
	{
		string token = split_tokens[token_index];
		if (token.size() == 0)
			continue;

		bool does_match = !(grammar.word_map.count(token) == 0);

		if (does_match)
		{
			vector<Frame> word_frames_identified = grammar.word_map.at(token);

			word_frames.push_back(word_frames_identified[0]);
		}

		if (token_index != split_tokens.size() - 1)
			token += ' '; // if not last token, add a space

		if (parser.is_highlighted && is_word_highlighted[token_index])
		{
			cv::Size size = measure_text(token);
			rectangle(img, ticker_text_corner, ticker_text_corner + Point(size.width, -size.height), HIGHLIGHTER_YELLOW, cv::FILLED);
		}

		if (!does_match)
		{
			// not present
			display_text(img, ticker_text_corner, token, CV_RGB(255, 10, 10));
		}
		else
		{
			display_text(img, ticker_text_corner, token, CV_RGB(118, 185, 0)); // draw token in green
		}

		// update text_corner position
		ticker_text_corner += Point(measure_text(token).width, 0);
	}

	int total_tokenized_width = ticker_text_corner.x - start_text_corner.x;

	// display the text cursor
	Size text_size = measure_text(current_utterance);

	Point cursor_top = start_text_corner + Point(total_tokenized_width, 0);
	Point cursor_bottom = cursor_top + Point(0, -text_size.height);

	cv::line(img, cursor_top, cursor_bottom, CV_RGB(200, 20, 20), 2, cv::LINE_4, 0);

	cv::imshow("reader", img);
}

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
				pair<Point, Point> cell_bounds = get_cell_bounds(start_grid_corner, row, col);
				if (is_in_bounds(Point(x, y), cell_bounds))
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
		display();
	}
	else if (event == EVENT_RBUTTONDOWN)
	{
	}
	else if (event == EVENT_MBUTTONDOWN)
	{
	}
	else if (event == EVENT_MOUSEMOVE)
	{
	}
}

int main(int argc, char **argv)
{
	namedWindow(screen_name, 1);

	// set the callback function for any mouse event
	setMouseCallback(screen_name, mouse_callback_function, NULL);
	resizeWindow(screen_name, 1024, 512);

	// Mat image;
	// read the grammar
	GrammarReader reader = GrammarReader(&grammar);
	reader.read_grammar("grammar.txt");

	// translate the read frames into cnf frames
	grammar.binarize_grammar();

	parser = Parser(grammar);

	parser.update_parse_grid(current_utterance);
	display();

	while (1)
	{
		if (check_keypress((char)waitKey(0)))
		{
			break;
		}

		// match word frames against the text
		display();
	}
	destroyAllWindows();

	return 0;
}
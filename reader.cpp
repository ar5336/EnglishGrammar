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

using namespace std;
using namespace cv;

string current_utterance = "";

Grammar grammar = Grammar();

// rows of columns of lists of frames
//  r3  X
//  r2  X  X
//  r1  X  X  X
//  r0  X  X  X  X
//      c0 c1 c2 c3

//       VP
//   NP
//  A   N    V
// the dog barked
vector<vector<vector<Frame>>> parse_grid;
pair<int, int> highlighted_cell_position;
bool is_highlighted = false;

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

bool does_frame_have_features(Frame candidate_frame, bool is_left, Frame &consumer_frame)
{
	vector<FeatureTag> feature_tags;
	int element_index;
	if (is_left)
	{
		element_index = 0;
	}
	else
	{
		element_index = 1;
	}
	PatternElement pattern_element = consumer_frame.pattern_elements[element_index];
	feature_tags = pattern_element.feature_tags;

	// first test the regular tags
	for (FeatureTag test_feature_tag : feature_tags)
	{
		set<string> tag_set = candidate_frame.feature_set;
		if (test_feature_tag.tag_type == FeatureTagType::Necessary)
		{
			// look for feature in frame
			if (tag_set.count(test_feature_tag.feature_name) == 0)
				return false;
		}
		else
		{
			// look for absence of feature in frame
			if (tag_set.count(test_feature_tag.feature_name) != 0)
				return false;
		}
	}

	vector<string> feature_group_tags = pattern_element.feature_group_tags;
	// then test for feature groups
	for (string feature_group_tag : feature_group_tags)
	{
		vector<string> features_to_check_for = grammar.feature_group_to_features.at(feature_group_tag);

		bool any_feature_matches_group = false;
		for (string feature_to_check_for : features_to_check_for)
		{
			if (candidate_frame.feature_set.count(feature_to_check_for) != 0)
			{
				any_feature_matches_group = true;
				// modify the consumer frame's tag. also modify the subsequent appearances of this feature group tag
				consumer_frame.feature_set.emplace(feature_to_check_for);
				if (element_index == 0)
				{
					// before appending a feature, check if the subsequent pattern element has a feature_group_tag
					bool does_have_feature_group = false;
					for (string potential_matching_feature_group : consumer_frame.pattern_elements[1].feature_group_tags)
					{
						if (equals(potential_matching_feature_group, feature_group_tag))
						{
							does_have_feature_group = true; // todo change the feature_group_tags to a set instead of a vector for perf improvement
							continue;
						}
					}
					if (does_have_feature_group)
						consumer_frame.pattern_elements[1].feature_tags.push_back(
							FeatureTag(
								feature_to_check_for,
								FeatureTagType::Necessary));
				}
				any_feature_matches_group = true;
			}
		}
		if (!any_feature_matches_group)
			return false; // consider changing this if the behavior is unexpected
	}
	return true;
}

bool get_matched_frames(Frame left_consumer_frame, Frame right_consumer_frame, vector<Frame> &matched_frames)
{ // update to vector<Frame>
	string left_string;
	if (left_consumer_frame.is_word_frame())
	{
		// is a word
		left_string = left_consumer_frame.get_part_of_speech();
	}
	else
	{
		left_string = left_consumer_frame.frame_name;
	}

	string right_string;
	if (right_consumer_frame.is_word_frame())
	{
		// is a word
		right_string = right_consumer_frame.get_part_of_speech();
	}
	else
	{
		right_string = right_consumer_frame.frame_name;
	}

	string match_string = left_string + " " + right_string;
	// printf("finding matching frames - '%s'\n", match_string.c_str());

	// printf("match string: %s\n", match_string.c_str());
	if (!(grammar.cnf_map.find(match_string) == grammar.cnf_map.end()))
	{
		vector<Frame> frames_to_doublecheck = grammar.cnf_map.at(match_string);

		// vector<Frame> accepted_frames;
		for (int frame_index = 0; frame_index < frames_to_doublecheck.size(); frame_index++)
		{
			Frame candidate_frame = frames_to_doublecheck[frame_index];

			// perform checks on PoS type and features

			// feature tag check
			PatternElement left_pattern_element = candidate_frame.pattern_elements[0];
			PatternElement right_pattern_element = candidate_frame.pattern_elements[1];

			vector<FeatureTag> left_feature_tags = left_pattern_element.feature_tags;
			vector<FeatureTag> right_feature_tags = right_pattern_element.feature_tags;

			if (does_frame_have_features(left_consumer_frame, true, candidate_frame) && does_frame_have_features(right_consumer_frame, false, candidate_frame))
				matched_frames.push_back(candidate_frame);
		}

		// for (int i = 0; i < accepted_frames.size(); i++){
		// 	matched_frames.push_back(accepted_frames.at(i));
		// }
		return true;
		// printf("\tthe frame's name: %s\n", found_frame.frame_name.c_str());
	}
	return false;
}

vector<Frame> find_matching_frames(vector<Frame> left_frames, vector<Frame> right_frames)
{
	vector<Frame> matching_frames;
	for (int left_index = 0; left_index < left_frames.size(); left_index++)
	{
		Frame left_frame = left_frames.at(left_index);
		for (int right_index = 0; right_index < right_frames.size(); right_index++)
		{
			Frame right_frame = right_frames.at(right_index);

			vector<Frame> matched_frames;
			if (get_matched_frames(left_frame, right_frame, matched_frames))
			{
				for (int match_index = 0; match_index < matched_frames.size(); match_index++)
				{
					matching_frames.push_back(matched_frames.at(match_index));
				}
				// printf("found %ld matchs\n", matched_frames.size());
			}
		}
	}

	return matching_frames;
}

void update_parse_grid()
{
	// tokenize the utterance
	vector<string> split_tokens;
	boost::split(split_tokens, current_utterance, boost::is_any_of(" "), boost::token_compress_on);
	int token_count = split_tokens.size();

	// initialize the parse_grid
	vector<vector<vector<Frame>>> new_grid;
	for (int row = 0; row < token_count; row++)
	{
		vector<vector<Frame>> new_row;
		for (int col = 0; col < token_count - row; col++)
		{
			vector<Frame> new_cell;
			new_row.push_back(new_cell);
		}
		new_grid.push_back(new_row);
	}

	parse_grid = new_grid;

	// populate the bottom row with the word frames of the utterance
	for (int token_index = 0; token_index < token_count; token_index++)
	{
		string token = split_tokens[token_index];
		bool does_match = !(grammar.word_map.find(token) == grammar.word_map.end());

		if (does_match)
		{
			vector<Frame> word_frames_identified = grammar.word_map.at(token);
			for (Frame word_frame : word_frames_identified)
			{
				parse_grid[0][token_index].push_back(word_frame);
			}
		}
	}

	if (token_count < 2)
		return;

	// perform cyk algo
	printf("parsing grammar\n");
	for (int row = 1; row < token_count; row++)
	{
		for (int col = 0; col < token_count - row; col++)
		{
			vector<Frame> potential_frames;

			// use the cnf

			// X         4
			// 0 3       3
			// 1   2     2
			// 2_____1   1
			// 3       0 0

			// X__
			// L  R

			for (int pair_index = 0; pair_index < row; pair_index++)
			{
				int left_row = row - (pair_index + 1);
				vector<Frame> left_frames = parse_grid[left_row][col];

				// for (int right_index = 0; right_index < row; right_index++){
				int step_diagonal = row - pair_index;
				int right_row = row - step_diagonal;
				int right_col = col + step_diagonal;
				vector<Frame> right_frames = parse_grid[right_row][right_col];

				vector<Frame> matching_frames = find_matching_frames(left_frames, right_frames);
				for (Frame matching_frame : matching_frames)
				{
					// printf("adding matched frame named %s. to row %d, col %d\n", matched_frame.frame_name.c_str(), row, col);
					parse_grid[row][col].push_back(matching_frame);
				}
				// }
			}
		}
	}
}

bool check_keypress(char cr)
{
	is_highlighted = false;
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
			update_parse_grid();

			// display();
		}

		if (cr == 8)
		{ // backspace{
			int utterance_size = current_utterance.size();
			if (utterance_size >= 1)
				current_utterance = current_utterance.substr(0, utterance_size - 1);
			update_parse_grid();
		}
		if (cr == 13)
		{ // enter
			// update the cyk grid with the latest utterance
			update_parse_grid();
		}
		if (cr == '\'')
		{
			current_utterance += '\'';
		}

		return false;
	}
}

//
//
//
//
// (A)  B  (C)  D

//    1
//   0 \
//  / \ \
// A (B) C

// 0 > A B
// 1 > 0 C
//   > A C
void binarize_grammar()
{ // first version with assumption of no optional frames - to be updated.
	printf("binarizing grammar\n");
	for (int frame_index = 0; frame_index < grammar.syntax_frames.size(); frame_index++)
	{
		vector<Frame> cnf_subframes;

		Frame frame = grammar.syntax_frames.at(frame_index);

		int pattern_length = frame.pattern_elements.size();
		int num_subframes = pattern_length - 1;

		vector<PatternElement> base_pattern = frame.pattern_elements;
		string base_frame_name = frame.frame_name;
		string base_frame_nickname = frame.frame_nickname;
		set<string> base_frame_feature_set = frame.feature_set;
		set<string> base_frame_feature_groups = frame.feature_groups;

		// create subframe 0 for pattern elements 0 and 1

		//      AL      - AL > 1 C
		//     / \      -  1 > A B
		//    1   \     
		//   / \   \    
		//  A   B   C

		//        AL       - AL > 1 D
		//       / \       -  1 > 2 C
		//      1   \      -  2 > A B
		//     / \   \
		//    2   \   \
		//   / \   \   \
		//  A   B   C   D

		// create subframes for subsequent elements
		for (int subframe_index = 0; subframe_index < num_subframes; subframe_index++)
		{
			set<string> feature_set;
			set<string> feature_groups;

			string frame_name;
			string frame_nickname;
			if (subframe_index == 0)
			{ // the base frame
				frame_name = base_frame_name;
				feature_set = base_frame_feature_set;
				feature_groups = base_frame_feature_groups;
				frame_nickname = base_frame_nickname;
			}
			else
			{ // a product of binarization
				frame_name = base_frame_name + to_string(subframe_index);
				frame_nickname = base_frame_nickname + to_string(subframe_index);
				feature_groups = base_frame_feature_groups;
			}

			PatternElement pattern_right = base_pattern.at(base_pattern.size() - 1 - subframe_index);
			PatternElement pattern_left;
			if (subframe_index == num_subframes - 1)
			{
				pattern_left = base_pattern.at(0);
			}
			else
			{
				pattern_left.match_string = base_frame_name + to_string(subframe_index + 1);
			}
			// printf("CNF: %s > %s %s\n", frame_name.c_str(), pattern_left.match_string.c_str(), pattern_right.match_string.c_str());

			Frame new_cnf_frame = Frame(
				frame_name,
				frame_nickname,
				pattern_left,
				pattern_right,
				feature_set,
				feature_groups);
			grammar.cnf_frames.push_back(new_cnf_frame);

			// add elements to cnf_map
			string match_pattern = pattern_left.match_string + " " + pattern_right.match_string;
			if (!(grammar.cnf_map.find(match_pattern) == grammar.cnf_map.end()))
			{
				grammar.cnf_map.at(match_pattern).push_back(new_cnf_frame);
			}
			else
			{
				vector<Frame> new_frame_vec;
				new_frame_vec.push_back(new_cnf_frame);
				grammar.cnf_map.emplace(match_pattern, new_frame_vec);
			}
		}
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

	if (!parse_grid.empty() && token_count != 0)
	{
		// diplay and initialize parse grid

		//  r3  X
		//  r2  X  X
		//  r1  X  X  X
		//  r0  X  X  X  X
		//      c0 c1 c2 c3

		int cell_width = 80;
		int cell_height = 20;

		for (int row = 0; row < parse_grid.size(); row++)
		{
			for (int col = 0; col < parse_grid.at(row).size(); col++)
			{

				pair<Point, Point> cell_bounds = get_cell_bounds(start_grid_corner, row, col);

				Point top_left = cell_bounds.first;
				Point bottom_right = cell_bounds.second;

				bool is_covered_by_highlight;
				bool is_this_cell_selected;
				if (is_highlighted)
				{
					int highlight_row = highlighted_cell_position.first;
					int highlight_col = highlighted_cell_position.second;
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

				vector<Frame> frames_in_cell = parse_grid.at(row).at(col);

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

		if (is_highlighted && is_word_highlighted[token_index])
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
		for (int row = 0; row < parse_grid.size() && !highlight_found; row++)
		{
			for (int col = 0; col < parse_grid.at(row).size() && !highlight_found; col++)
			{
				pair<Point, Point> cell_bounds = get_cell_bounds(start_grid_corner, row, col);
				if (is_in_bounds(Point(x, y), cell_bounds))
				{
					highlighted_cell_position = pair<int, int>(row, col);
					highlight_found = true;
					is_highlighted = true;
					break;
				}
			}
		}

		if (!highlight_found)
		{
			is_highlighted = false;
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
	binarize_grammar();

	update_parse_grid();
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
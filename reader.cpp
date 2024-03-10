// #include <iostream>
// #include <iomanip>
#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include </usr/local/include/opencv4/opencv2/tracking.hpp>
#include </usr/local/include/opencv4/opencv2/videoio/videoio_c.h>
#include <vector>
// #include <unordered_set>
#include <map>
// #include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <set>
// #include <set>
// // #include <bits/stdc++.h>

using namespace std;
using namespace cv;

string current_utterance = "";

bool equals(string a, string b)
{
	return a.compare(b) == 0;
}

enum PatternNecessity
{
	Required = 1,
	Optional = 2,
};

enum FeatureTagType
{
	Necessary = 1,
	Prohibited = 2,
};

class FeatureTag
{
public:
	string feature_name;
	FeatureTagType tag_type;

	FeatureTag(string name, FeatureTagType type)
	: feature_name(name), tag_type(type){

	}
};

class PatternElement
{
public:
	string match_string;
	string pattern_true_match_type;
	PatternNecessity pattern;
	vector<FeatureTag> feature_tags;

	// constructor for intermediate frame
	PatternElement(string match_string)
	: match_string(match_string)
	{}

	// complete constructor
	PatternElement(
		string match_string,
		string pattern_true_match_type,
		PatternNecessity pattern,
		vector<FeatureTag> feature_tags)
	: match_string(match_string),
	  pattern_true_match_type(pattern_true_match_type),
	  pattern(pattern),
	  feature_tags(feature_tags)
	{}
};

class Frame
{
public:
	string frame_name;
	string frame_nickname;
	vector<string> type_heirarchy;
	vector<string> pattern;			   // a list of parts of speech or Frame names
	vector<PatternNecessity> pattern_types; // whether those parts are optional or not. - not implemented during binarization yet
	vector<FeatureTag> pattern_feature_tags;

	set<string> feature_set; // form of Word

	// default constructor
	Frame() {}

	// word frame constructor
	Frame(vector<string> type_heirarchy,
		string word_form)
		: type_heirarchy(type_heirarchy)
	{
		// pattern and pattern_form are left null
		feature_set.emplace(word_form);
	}

	// word frame with multiple features
	Frame(vector<string> type_heirarchy,
		vector<string> features)
		: type_heirarchy(type_heirarchy)
	{
		// pattern and pattern_form are left null
		for(int i = 0; i < features.size(); i++){
			feature_set.emplace(features.at(i));
		}
		feature_set.insert(type_heirarchy.begin(), type_heirarchy.end());
	}

	// featureless word frame constructor
	Frame(vector<string> type_heirarchy)
		: type_heirarchy(type_heirarchy)
	{
	}

	// syntax frame constructor
	Frame(
		string frame_name,
		string frame_nickname,
		// set<string> type_set,
		vector<string> pattern,
		vector<PatternNecessity> pattern_types)
		: frame_name(frame_name),
		  frame_nickname(frame_nickname),
		//   type_set(type_set),
		  pattern(pattern),
		  pattern_types(pattern_types)
	{
		// features not implemented for syntax frames yet
	}

	// cnf frame constructor
	Frame(
		string frame_name,
		// set<string> type_set,
		string left,
		string right)
		: frame_name(frame_name),
		//   type_set(type_set),
		  pattern(pattern),
		  pattern_types(vector<PatternNecessity>())
	{
		pattern.push_back(left);
		pattern.push_back(right);
		// features not implemented for syntax frames yet
	}

	// vector<string> get_feature_vector()
	// {
	// 	vector<string> type_list = vector<string>(type_set.begin(), type_set.end());
	// 	return type_list;
	// }

	string get_part_of_speech()
	{
		return type_heirarchy.at(0);
	}

	// bool is_consumed(string pattern_name)
	// {
	// 	return !(type_set.find(pattern_name) == type_set.end());
	// }

	bool is_word_frame(){
		return !type_heirarchy.empty();
	}
};

map<string, Frame> word_map;
map<string, string> base_pos_to_type_map; // not used yet - good for advanced parsing efficiency
map<string, set<Frame>> pos_map; // not used yet - good for advanced parsing efficiency

// map<string, string> syntax_nickname_to_name_map;
vector<Frame> syntax_frames;
map<string, Frame> syntax_name_map;

vector<Frame> cnf_frames;
map<string, vector<Frame>> cnf_map; // frame A > B C becomes map entry {"B C", "A"}
// map<int, vector<Frame>> pattern_length_map; // unused yet
// map<set<string>, vector<Frame>> pattern_map; // not used at the moment - good for efficiency when parsing

// map<string, vector<Frame>> cnf_map;

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

// vector<Frame> get_matched_frames(Frame left, Frame right){ // update to vector<Frame>
// 	string left_string;
// 	if (left.is_word_frame()){
// 		// is a word
// 		left_string = left.get_part_of_speech();
// 	} else {
// 		left_string = left.frame_name;
// 	}

// 	string right_string;
// 	if (right.is_word_frame()){
// 		// is a word
// 		right_string = right.get_part_of_speech();
// 	} else {
// 		right_string = right.frame_name;
// 	}

// 	string match_string = left_string + " " + right_string;
// 	vector<Frame> return_vec;
// 	return_vec.push_back(Frame());
// 	// printf("match string: %s\n", match_string.c_str());
// 	if (!(cnf_map.find(match_string) == cnf_map.end())){
// 		return cnf_map.at(match_string);
// 		// printf("\tthe frame's name: %s\n", found_frame.frame_name.c_str());
// 	}
// 	return return_vec;
// }

bool get_matched_frames(Frame left, Frame right, vector<Frame>& matched_frames){ // update to vector<Frame>
	string left_string;
	if (left.is_word_frame()){
		// is a word
		left_string = left.get_part_of_speech();
	} else {
		left_string = left.frame_name;
	}

	string right_string;
	if (right.is_word_frame()){
		// is a word
		right_string = right.get_part_of_speech();
	} else {
		right_string = right.frame_name;
	}

	string match_string = left_string + " " + right_string;
	// printf("match string: %s\n", match_string.c_str());
	if (!(cnf_map.find(match_string) == cnf_map.end())){
		vector<Frame> frames_to_doublecheck = cnf_map.at(match_string);

		// perform checks on PoS type and features

		for (int i = 0; i < frames_to_doublecheck.size(); i++){
			matched_frames.push_back(frames_to_doublecheck.at(i));
		}
		return true;
		// printf("\tthe frame's name: %s\n", found_frame.frame_name.c_str());
	}
	return false;
}

vector<Frame> find_matching_frames(vector<Frame> left_frames, vector<Frame> right_frames)
{
	vector<Frame> matching_frames;
	for (int left_index = 0; left_index < left_frames.size(); left_index++){
		Frame left_frame = left_frames.at(left_index);
		for (int right_index = 0; right_index < right_frames.size(); right_index++){
			Frame right_frame = right_frames.at(right_index);

			vector<Frame> matched_frames;
			if (get_matched_frames(left_frame, right_frame, matched_frames)){
				for(int match_index = 0; match_index < matched_frames.size(); match_index++){
					matching_frames.push_back(matched_frames.at(match_index));
				}
				printf("found %ld matchs\n", matched_frames.size());
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
		bool does_match = !(word_map.find(token) == word_map.end());

		if (does_match)
		{
			Frame word_frame_identified = word_map.at(token);
			parse_grid.at(0).at(token_index).push_back(word_frame_identified);
		}
	}

	if (token_count < 2)
		return;

	// perform cyk algo
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

			

			for (int pair_index = 0; pair_index < row; pair_index++){
				int left_row = row - (pair_index + 1);
				vector<Frame> left_frames = parse_grid.at(left_row).at(col);

				// for (int right_index = 0; right_index < row; right_index++){
					int step_diagonal = row - pair_index;
					int right_row = row - step_diagonal;
					int right_col = col + step_diagonal;
					vector<Frame> right_frames = parse_grid.at(right_row).at(right_col);

					vector<Frame> matching_frames = find_matching_frames(left_frames, right_frames);
					for (int matched_index = 0; matched_index < matching_frames.size(); matched_index++){
						Frame matched_frame = matching_frames.at(matched_index);
						printf("adding matched frame named %s. to row %d, col %d\n", matched_frame.frame_name.c_str(), row, col);
						parse_grid.at(row).at(col).push_back(matched_frame);
					}
				// }
			}

		}
	}
}

bool check_keypress(char cr)
{
	if (cr == 27)
	{
		return true; // break
	}
	else
	{
		// printf("character: %d\n", cr);
		if (cr >= 97 && cr <= 122 || cr == 32)
			current_utterance += cr;
		if (cr == 8)
		{ // backspace{
			int utterance_size = current_utterance.size();
			if (utterance_size >= 1)
				current_utterance = current_utterance.substr(0, utterance_size - 1);
		}
		if (cr == 13)
		{ // enter
			// update the cyk grid with the latest utterance
			update_parse_grid();
		}

		return false;
	}
}

inline void ltrim(string &s)
{
	s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch)
							   { return !isspace(ch); }));
}

// trim from end (in place)
inline void rtrim(string &s)
{
	s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch)
					{ return !isspace(ch); })
				.base(),
			s.end());
}

inline void trim(string &s)
{
	rtrim(s);
	ltrim(s);
}

int count_initial_spaces(string str)
{
	char initial_char = str.at(0);
	int char_index = 0;
	while (initial_char == ' ')
	{
		char_index++;
		initial_char = str.at(char_index);
	}
	return char_index - 1;
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
	for (int frame_index = 0; frame_index < syntax_frames.size(); frame_index++)
	{
		vector<Frame> cnf_subframes;

		Frame frame = syntax_frames.at(frame_index);

		int pattern_length = frame.pattern.size();
		int num_subframes = pattern_length - 1;

		vector<string> base_pattern = frame.pattern;
		string base_frame_name = frame.frame_name;
		// set<string> base_typeset = frame.type_set;
		// string base_frame_name = frame.frame_nickname;

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
			string frame_name;
			if (subframe_index == 0)
			{
				frame_name = base_frame_name;
			}
			else
			{
				frame_name = base_frame_name+to_string(subframe_index);
			}
			string pattern_right = base_pattern.at(base_pattern.size() - 1 - subframe_index);
			string pattern_left;
			if (subframe_index == num_subframes - 1)
			{
				pattern_left = base_pattern.at(0);
			}
			else
			{
				pattern_left = base_frame_name+to_string(subframe_index + 1);
			}
			printf("CNF: %s > %s %s\n", frame_name.c_str(), pattern_left.c_str(), pattern_right.c_str());

			Frame new_cnf_frame = Frame(frame_name, pattern_left, pattern_right);
			cnf_frames.push_back(new_cnf_frame);

			// add elements to cnf_map
			string match_pattern = pattern_left + " " + pattern_right;
			if (!(cnf_map.find(match_pattern) == cnf_map.end())){
				cnf_map.at(match_pattern).push_back(new_cnf_frame);

			}
			else {
				vector<Frame> new_frame_vec;
				new_frame_vec.push_back(new_cnf_frame);
				cnf_map.emplace(match_pattern, new_frame_vec);
			}

			// while (subframe_index > 0)
			// Frame new_cnf_frame = Frame(frame.frame_nickname + to_string(subframe_index),
			// 	frame.type_set,
			// 	base_pattern.)
		}
	}
}

void read_grammar(string fileName)
{
	fstream newfile;
	int tab_spaces = 4;

	newfile.open(fileName, ios::in); // open a file to perform read operation using file object
	if (newfile.is_open())
	{ // checking whether the file is open
		bool reading_syntax = false;
		string current_line;

		int previous_indentation = 0;

		vector<PatternNecessity> term_forms;
		vector<string> term_form_names;

		vector<string> type_heirarchy;

		string pattern_name;
		string pattern_nickname;

		while (getline(newfile, current_line))
		{ // read data from file object and put it into string.

			// bool subcategory = false;
			if (current_line.size() == 0)
				continue;

			// printf("flag9\n");

			// measure indentation by counting initial spaces
			int initial_spaces = count_initial_spaces(current_line);
			int current_indentation = initial_spaces / tab_spaces;

			// if (current_indentation > previous_indentation){
			// 	subcategory = true;
			// } else
			if (current_indentation < previous_indentation)
			{
				for (int i = 0; i < previous_indentation - current_indentation; i++)
				{
					if (type_heirarchy.size() > 0)
					{
						type_heirarchy.pop_back();
					}
					term_forms.clear();
					term_form_names.clear();
				}
			}
			previous_indentation = current_indentation;

			// printf("flag10\n");

			trim(current_line);

			if (equals(current_line, "Frames:"))
			{
				reading_syntax = true;
			}

			vector<string> split_tokens;
			boost::split(split_tokens, current_line, boost::is_any_of(" "), boost::token_compress_on);

			// if the first token ends in a ":" you're going up a level in the type_heirarchy
			string first_token = split_tokens[0];
			if (first_token.at(first_token.size() - 1) == ':')
			{
				// reading a type, to be followed by indent
				type_heirarchy.push_back(first_token.substr(0, first_token.size() - 1));

				// check for formlist formatted like this:
				// 		PoSType: #form1 form2 (form3)
				if (split_tokens.size() > 1 && split_tokens[1].at(0) == '#')

					for (int i = 2; i < split_tokens.size(); i++)
					{
						string term_form_string = split_tokens[i];
						if (term_form_string.at(0) == '(' && term_form_string.back() == ')')
						{
							// is optional
							term_forms.push_back(PatternNecessity::Optional);
							term_form_names.push_back(term_form_string.substr(1, term_form_string.size() - 1));
						}
						// is required
						term_forms.push_back(PatternNecessity::Required);
						term_form_names.push_back(term_form_string);
					}
			}
			else
			{
				// reading an entry
				if (reading_syntax)
				{
					// determine if you're reading a name / nickname line or a pattern line
					// the second token of a pattern name line is quoted
					string second_token = split_tokens[1];
					bool is_pattern_name_line = (second_token.at(0) == '"' && second_token.back() == '"');

					if (is_pattern_name_line)
					{
						// printf("is name line\n");
						pattern_name = first_token;
						pattern_nickname = second_token.substr(1, second_token.size() - 1);
					}
					else
					{
						vector<string> pattern;
						vector<PatternNecessity> pattern_types;
						for (int pattern_element_index = 0; pattern_element_index < split_tokens.size(); pattern_element_index++)
						{
							string pattern_element = split_tokens[pattern_element_index];
							bool is_optional = (pattern_element.at(0) == '(' && pattern_element.back() == ')');
							
							if (is_optional)
							{
								string no_parens = pattern_element.substr(1, pattern_element.size() - 2);
								// printf("item %d: (%s\n", pattern_element_index, no_parens.c_str());
								pattern.push_back(no_parens);
								pattern_types.push_back(PatternNecessity::Optional);
							}
							else
							{
								// printf("item %d: %s\n", pattern_element_index, pattern_element.c_str());
								pattern.push_back(pattern_element);
								pattern_element.push_back(PatternNecessity::Required);
							}
						}

						// set<string> type_set,
						//  vector<string> pattern,
						//  vector<PatternType> pattern_types)

						// printf("flag7.5\n");

						Frame new_pattern_frame = Frame(
							pattern_name,
							pattern_nickname,
							pattern,
							pattern_types);

						// printf("flag8\n");
						syntax_frames.push_back(new_pattern_frame);
						// printf("flag9\n");

						// syntax_name_map.emplace(pattern_name, new_pattern_frame);
					}
				}
				else
				{
					// reading word

					// printf("flag11\n");

					// printf("type_heirarchy size: %ld\n", type_heirarchy.size());
					// have a copy type_heirarchy for inserting
					vector<string> type_pruned = vector<string>(type_heirarchy.begin() + 1, type_heirarchy.end());
					// vector<string> type_heirarchy = type_pruned.begin(), type_pruned.end();

					// printf("flag12\n");

					// otherwise it is a list of word forms that corresponds to the current term
					// therefore, populate the hash set of words with the newly created word object
					string base_word = "";
					if (term_forms.size() == 0)
					{
						word_map.emplace(split_tokens[0], Frame(type_pruned));
					}
					else
					{
						if (split_tokens.size() > 1 && split_tokens.at(1).at(0) == '-'){
							// reading a word with feature markers (should not have had word for indicated by #list)
							string word_string = split_tokens[0];
							vector<string> features;
							for (int i = 1; i < split_tokens.size(); i++){
								string feature_name = split_tokens[i];
								features.push_back(feature_name.substr(1, feature_name.size()));
							}
							Frame new_word_frame = Frame(type_pruned, features);

							word_map.emplace(word_string, new_word_frame);
						}
						else {
							for (int word_form_index = 0; word_form_index < split_tokens.size(); word_form_index++)
							{
								// if the first char of this 'word' is '-' then it and subsequent tokens are feature markers
								string wordString = split_tokens[word_form_index];

								if (word_form_index == 0)
								{
									// is base form
									base_word = wordString;
								}
								string word_form = term_form_names.at(word_form_index);
								Frame new_word_frame = Frame(type_pruned, word_form);

								word_map.emplace(wordString, new_word_frame);
							}
						}
					}
				}
			}

			// for (auto i : split_tokens)
			// 	cout << i << '_';
			// cout << "\n";

			// // cout << tp << "\n"; //print the data of the string
		}
		newfile.close(); // close the file object.
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

Size measure_text(string text)
{
	int was_found;
	return getTextSize(text,
					   cv::FONT_HERSHEY_TRIPLEX,
					   1.0,
					   2,
					   &was_found);
}

int main(int argc, char **argv)
{
	// Mat image;
	// read the grammar
	read_grammar("grammar.txt");

	// translate the read frames into cnf frames
	binarize_grammar();

	Mat img(512, 1024, CV_8UC3, cv::Scalar(0));

	Point start_text_corner = cv::Point(10, img.rows * 3 / 4); // top-left position

	while (1)
	{
		if (check_keypress((char)waitKey(0)))
		{
			break;
		}

		// match word frames against the text

		img.setTo(Scalar(0)); // clear screen

		vector<string> split_tokens;
		vector<Frame> word_frames;
		boost::split(split_tokens, current_utterance, boost::is_any_of(" "), boost::token_compress_on);

		int token_count = split_tokens.size();

		// display the text
		Point ticker_text_corner = start_text_corner;
		for (int token_index = 0; token_index < split_tokens.size(); token_index++)
		{
			string token = split_tokens[token_index];
			if (token.size() == 0)
				continue;

			bool does_match = !(word_map.find(token) == word_map.end());

			Point above_token_start = ticker_text_corner + Point(0, -30);
			if (does_match)
			{
				Frame word_frame_identified = word_map.at(token);
				word_frames.push_back(word_frame_identified);
				// set<string> PoS_set = word_frame_identified.type_set;

				// vector<string> PoS_list = vector<string>(PoS_set.begin(), PoS_set.end());

				// int PoS_type_number = PoS_list.size();
				// // diplay the part of speech
				// for (int i = 0; i < PoS_type_number; i++)
				// {
				// 	string PoS = PoS_list.at(i);

				// 	Point above_token = above_token_start + Point(0, -20 * i);
				// 	display_text(img, above_token, PoS, CV_RGB(100, 100, 200), 0.4f);
				// }
			}

			if (token_index != split_tokens.size() - 1)
				token += ' '; // if not last token, add a space

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

		if (!parse_grid.empty() && token_count != 0) {
			// diplay and initialize parse grid

			//  r3  X
			//  r2  X  X
			//  r1  X  X  X
			//  r0  X  X  X  X
			//      c0 c1 c2 c3

			int cell_width = 80;
			int cell_height = 20;

			Point start_grid_corner = start_text_corner + Point(0, -60);

			for (int row = 0; row < parse_grid.size(); row++)
			{
				for (int col = 0; col < parse_grid.at(row).size(); col++){
					// printf("x ");
					Point top_left = start_grid_corner + Point(col * cell_width + (row * cell_width / 2), -(row * cell_height));
					Point bottom_right = top_left + Point(cell_width, cell_height);
					rectangle(img, top_left, bottom_right, CV_RGB(255, 255, 255));

					vector<Frame> frames_in_cell = parse_grid.at(row).at(col);

					Point bottom_left = top_left + Point(10, cell_height - 10);
					for(int frame_index = 0; frame_index < frames_in_cell.size(); frame_index++)
					{
						Frame frame = frames_in_cell.at(frame_index);
						// display word
						if (row == 0)
						{
							display_text(img, bottom_left, frame.get_part_of_speech(), CV_RGB(100, 100, 200), 0.65f);
						} else {
							// display frame
							// string frame_name =  
							display_text(img, bottom_left, frame.frame_name, CV_RGB(100, 200, 100), 0.65f);

						}

					}
				}
				// printf("\n");
			}
		}
		

		int total_tokenized_width = ticker_text_corner.x - start_text_corner.x;

		// display the text cursor
		Size text_size = measure_text(current_utterance);

		Point cursor_top = start_text_corner + Point(total_tokenized_width, 0);
		Point cursor_bottom = cursor_top + Point(0, -text_size.height);

		cv::line(img, cursor_top, cursor_bottom, CV_RGB(200, 20, 20), 2, cv::LINE_4, 0);

		cv::imshow("reader", img);
	}
	// When everything done, release the video capture object
	// cap.release();

	// Closes all the frames
	destroyAllWindows();

	return 0;
}
#ifndef FRAMES_HPP
#define FRAMES_HPP

#include <string>
#include <vector>
#include <set>

#include "predicate_rule_reader.hpp"

using namespace std;

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

enum FrameType
{
	Null = 0,
	Word = 1,
	Syntax = 2,
	Binarized = 3,
	Matched = 4,
	MonoFrame_Derived = 5,
	MultiFrame_Derived = 6,
	Reconstructed = 7, // not yet implemented
};

class FeatureTag
{
public:
	string feature_name;
	FeatureTagType tag_type;

	FeatureTag(string name, FeatureTagType type);
};

class PatternElement
{
public:
	string match_string;
	string pattern_true_match_type;
	PatternNecessity necessity;
	vector<FeatureTag> feature_tags;
	vector<string> feature_group_tags;

	PatternElement(const PatternElement& other);

	// default constructor
	PatternElement();

	// constructor for intermediate frame
	PatternElement(string match_string);

	// complete constructor
	PatternElement(
		string match_string,
		string pattern_true_match_type,
		PatternNecessity necessity,
		vector<FeatureTag> feature_tags,
		vector<string> feature_group_tags);

	string stringify();
};

class FrameCoordinates
{
public:
	int row, col, num;

	FrameCoordinates();

	FrameCoordinates(int row, int col, int num);

	bool is_empty();

	void print_out();

	string stringify();
};

bool operator<(const FrameCoordinates& lhs, const FrameCoordinates& rhs);

class Frame
{
public:
	FrameType type;
	int definition_line;

	string frame_name;
	string frame_nickname;
	vector<string> type_heirarchy;
	vector<PatternElement> pattern_elements;
	PredicateFormationRules predicate_formation_rules;

	set<string> feature_set; // feature applied to word or syntax pattern
	set<string> feature_groups;

	bool is_binarized;

	// there are only used when constructing interpretations
	FrameCoordinates left_match;
	FrameCoordinates right_match;

	Expression accumulated_expression;

	// this param is synechodche for the matched basis frame, which has no coordinates (yet)
	// if we want to access more than just the frame name, maybe the frame features
	// int original_frame_index;

	// default constructor
	Frame();

	// word frame constructor
	Frame(
		string frame_name,
		int definition_line,
		vector<string> type_heirarchy,
		string word_form);

	// word frame with multiple features
	Frame(
		string frame_name,
		int definition_line,
		vector<string> type_heirarchy,
		vector<string> features);

	// featureless word frame constructor
	Frame(
		string frame_name,
		int definition_line,
		vector<string> type_heirarchy);

	// syntax frame constructor
	Frame(
		string frame_name,
		int definition_line,
		string frame_nickname,
		vector<PatternElement> pattern_elements,
		set<string> feature_set,
		set<string> feature_groups,
		PredicateFormationRules formation_rules);

	// cnf frame constructor
	Frame(
		string frame_name,
		int definition_line,
		string frame_nickname,
		PatternElement left,
		PatternElement right,
		set<string> feature_set,
		set<string> feature_groups,
		PredicateFormationRules formation_rules);

	// matched frame constructor
	Frame(
		string frame_name,
		int definition_line,
		string frame_nickname,
		PatternElement left,
		PatternElement right,
		set<string> feature_set,
		set<string> feature_groups,
		FrameCoordinates left_match,
		FrameCoordinates right_match,
		Expression accumulated_expression);

	// matched monoframe constructor
	Frame(
		string frame_name,
		int definition_line,
		string frame_nickname,
		PatternElement mono_element,
		set<string> feature_set,
		set<string> feature_groups,
		FrameCoordinates left_match,
		Expression accumulated_expression);
	
	Frame with_links(
		FrameCoordinates to_left,
    	FrameCoordinates to_right);

	string stringify_as_param();

	string stringify_pattern_elements();

	string get_part_of_speech();

	bool is_part_of_speech(string part_of_speech);

	bool is_word_frame();

	bool is_matched();

	Frame with_expression(Expression expression);

	void print_out(string title);

	string stringify_pre_binarization();
};

bool operator<(const Frame& lhs, const Frame& rhs);

#endif
#ifndef FRAMES_HPP
#define FRAMES_HPP

#include "predicate_rule_reader.hpp"

#include <string>
#include <vector>
#include <set>

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
	Reconstructed = 5, // not yet implemented
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
};

class FrameCoordinates
{
public:
	int row, col, num;

	FrameCoordinates();

	FrameCoordinates(int row, int col, int num);

	bool is_empty();

	void print_out();
};

class Frame
{
public:
	FrameType type;

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

	// default constructor
	Frame();

	// word frame constructor
	Frame(
		string frame_name,
		vector<string> type_heirarchy,
		string word_form);

	// word frame with multiple features
	Frame(
		string frame_name,
		vector<string> type_heirarchy,
		vector<string> features);

	// featureless word frame constructor
	Frame(
		string frame_name,
		vector<string> type_heirarchy);

	// syntax frame constructor
	Frame(
		string frame_name,
		string frame_nickname,
		vector<PatternElement> pattern_elements,
		set<string> feature_set,
		set<string> feature_groups,
		PredicateFormationRules formation_rules);

	// cnf frame constructor
	Frame(
		string frame_name,
		string frame_nickname,
		PatternElement left,
		PatternElement right,
		set<string> feature_set,
		set<string> feature_groups,
		PredicateFormationRules formation_rules);

	// matched frame constructor
	Frame(
		string frame_name,
		string frame_nickname,
		PatternElement left,
		PatternElement right,
		set<string> feature_set,
		set<string> feature_groups,
		FrameCoordinates left_match,
		FrameCoordinates right_match,
		Expression accumulated_expression);
	
	Frame with_links(
		FrameCoordinates to_left,
    	FrameCoordinates to_right);

	string stringify_as_param();

	string get_part_of_speech();

	bool is_part_of_speech(string part_of_speech);

	bool is_word_frame();

	bool is_matched();

	Frame with_expression(Expression expression);

	void print_out(string title);
};

#endif
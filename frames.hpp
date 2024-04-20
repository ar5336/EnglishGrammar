#ifndef FRAMES_HPP
#define FRAMES_HPP

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

class Frame
{
public:
	string frame_name;
	string frame_nickname;
	vector<string> type_heirarchy;
	vector<PatternElement> pattern_elements;

	set<string> feature_set; // feature applied to word or syntax pattern
	set<string> feature_groups;

	// default constructor
	Frame();

	// word frame constructor
	Frame(vector<string> type_heirarchy,
		  string word_form);

	// word frame with multiple features
	Frame(vector<string> type_heirarchy,
		  vector<string> features);

	// featureless word frame constructor
	Frame(vector<string> type_heirarchy);

	// syntax frame constructor
	Frame(
		string frame_name,
		string frame_nickname,
		// vector<string> pattern,
		// vector<PatternNecessity> pattern_types
		vector<PatternElement> pattern_elements,
		set<string> feature_set,
		set<string> feature_groups);

	// cnf frame constructor
	Frame(
		string frame_name,
		string frame_nickname,
		// set<string> type_set,
		PatternElement left,
		PatternElement right,
		set<string> feature_set,
		set<string> feature_groups);

	string get_part_of_speech();

	bool is_part_of_speech(string part_of_speech);

	bool is_word_frame();
};

#endif
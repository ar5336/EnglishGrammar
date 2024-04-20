#include "frames.hpp"

FeatureTag::FeatureTag(string name, FeatureTagType type)
		: feature_name(name), tag_type(type) { }

// ======== PATTERN ELEMENT ========

// default constructor
PatternElement::PatternElement()
		: necessity(PatternNecessity::Required) { }

// constructor for intermediate frame
PatternElement::PatternElement(string match_string)
		: match_string(match_string),
		  necessity(PatternNecessity::Required) { }

// complete constructor
PatternElement::PatternElement(
		string match_string,
		string pattern_true_match_type,
		PatternNecessity necessity,
		vector<FeatureTag> feature_tags,
		vector<string> feature_group_tags)
		: match_string(match_string),
		  pattern_true_match_type(pattern_true_match_type),
		  necessity(necessity),
		  feature_tags(feature_tags),
		  feature_group_tags(feature_group_tags) { }


// ======== FRAME ========

// default constructor
Frame::Frame() {}

	// word frame constructor
Frame::Frame(vector<string> type_heirarchy,
        string word_form)
    : type_heirarchy(type_heirarchy),
        frame_name(word_form)
{
    // pattern and pattern_form are left null
    for (string type : type_heirarchy)
    {
        feature_set.insert(type);
    }

    feature_set.emplace(word_form);
}

	// word frame with multiple features
Frame::Frame(vector<string> type_heirarchy,
        vector<string> features)
    : type_heirarchy(type_heirarchy)
{
    // pattern and pattern_form are left null
    for (string feature : features)
    {
        feature_set.insert(feature);
    }

    for (string type : type_heirarchy)
    {
        feature_set.insert(type);
    }
}

	// featureless word frame constructor
Frame::Frame(vector<string> type_heirarchy)
    : type_heirarchy(type_heirarchy)
{
    for (string type : type_heirarchy)
    {
        feature_set.insert(type);
    }
}

	// syntax frame constructor
Frame::Frame(
    string frame_name,
    string frame_nickname,
    // vector<string> pattern,
    // vector<PatternNecessity> pattern_types
    vector<PatternElement> pattern_elements,
    set<string> feature_set,
    set<string> feature_groups)
    : frame_name(frame_name),
        frame_nickname(frame_nickname),
        pattern_elements(pattern_elements),
        feature_set(feature_set),
        feature_groups(feature_groups)
{
}

	// cnf frame constructor
Frame::Frame(
    string frame_name,
    string frame_nickname,
    // set<string> type_set,
    PatternElement left,
    PatternElement right,
    set<string> feature_set,
    set<string> feature_groups)
    : frame_name(frame_name),
        frame_nickname(frame_nickname),
        feature_set(feature_set),
        feature_groups(feature_groups)
//   type_set(type_set),
{
    pattern_elements.push_back(left);
    pattern_elements.push_back(right);
    // features not implemented for syntax frames yet
}

string Frame::get_part_of_speech()
{
    return type_heirarchy.at(0);
}

bool Frame::is_part_of_speech(string part_of_speech)
{
    return feature_set.count(part_of_speech) != 0;
}

bool Frame::is_word_frame()
{
    return !type_heirarchy.empty();
}


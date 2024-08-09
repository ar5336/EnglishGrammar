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


// ======== FRAMELINK ========

FrameCoordinates::FrameCoordinates()
{
    row = -1, col = -1, num = -1;
}

FrameCoordinates::FrameCoordinates(int row, int col, int num)
    : row(row), col(col), num(num)
    { }

bool FrameCoordinates::is_empty()
{
    return row == -1 || col == -1 || num == -1;
}

void FrameCoordinates::print_out()
{
    printf("\t\t row: %d, col: %d, num: %d\n", row, col, num);
}


// ======== FRAME ========

string Frame::stringify_as_param()
{
    if (is_word_frame())
    {
        return frame_name;
    }
    if (is_matched())
    {
        return frame_name;
    }
    return "BLARG";
}

// default constructor
Frame::Frame() {
    left_match = FrameCoordinates();
    right_match = FrameCoordinates();

    type = FrameType::Null;
}

// word frame constructor
Frame::Frame(
    string frame_name,
    vector<string> type_heirarchy,
    string word_form)
    : 
        frame_name(frame_name),
        type_heirarchy(type_heirarchy)
{
    is_binarized = false;
    
    // pattern and pattern_form are left null
    for (string type : type_heirarchy)
    {
        feature_set.insert(type);
    }

    feature_set.emplace(word_form);
    left_match = FrameCoordinates();
    right_match = FrameCoordinates();

    type = FrameType::Word;
}

// word frame with multiple features
Frame::Frame(
    string frame_name,
    vector<string> type_heirarchy,
    vector<string> features)
    : type_heirarchy(type_heirarchy),
        frame_name(frame_name)
{
    is_binarized = false;
    // pattern and pattern_form are left null
    for (string feature : features)
    {
        feature_set.insert(feature);
    }

    for (string type : type_heirarchy)
    {
        feature_set.insert(type);
    }

    type = FrameType::Word;
}

// featureless word frame constructor
Frame::Frame(
    string frame_name,   
    vector<string> type_heirarchy)
    : type_heirarchy(type_heirarchy),
        frame_name(frame_name)
{
    is_binarized = false;
    for (string type : type_heirarchy)
    {
        feature_set.insert(type);
    }

    type = FrameType::Word;
}

// syntax frame constructor
Frame::Frame(
    string frame_name,
    string frame_nickname,
    vector<PatternElement> pattern_elements,
    set<string> feature_set,
    set<string> feature_groups,
    PredicateFormationRules formation_rules)
    : frame_name(frame_name),
        frame_nickname(frame_nickname),
        pattern_elements(pattern_elements),
        feature_set(feature_set),
        feature_groups(feature_groups),
        predicate_formation_rules(formation_rules)
{
    is_binarized = false;

    type = FrameType::Syntax;
}

// cnf frame constructor
Frame::Frame(
    string frame_name,
    string frame_nickname,
    PatternElement left,
    PatternElement right,
    set<string> feature_set,
    set<string> feature_groups,
    PredicateFormationRules formation_rules)
    : frame_name(frame_name),
        frame_nickname(frame_nickname),
        feature_set(feature_set),
        feature_groups(feature_groups),
        predicate_formation_rules(formation_rules)
{
    is_binarized = true;
    pattern_elements.push_back(left);
    pattern_elements.push_back(right);
    // features not implemented for syntax frames yet

    left_match = FrameCoordinates();
    right_match = FrameCoordinates();
    // left_match = NULL;
    // right_match = &(Frame());

    type = FrameType::Binarized;
}

//  matched frame constructor
Frame::Frame(
    string frame_name,
    string frame_nickname,
    PatternElement left,
    PatternElement right,
    set<string> feature_set,
    set<string> feature_groups,
    FrameCoordinates left_match,
    FrameCoordinates right_match,
    Expression accumulated_expression)
    : frame_name(frame_name),
        frame_nickname(frame_nickname),
        feature_set(feature_set),
        feature_groups(feature_groups),
        left_match(left_match),
        right_match(right_match),
        accumulated_expression(accumulated_expression)
{
    is_binarized = true;
    pattern_elements.push_back(left);
    pattern_elements.push_back(right);
    // features not implemented for syntax frames yet

    // left_match = NULL;
    // right_match = &(Frame());
    // left_match = left_match;
    type = FrameType::Matched;
}

Frame Frame::with_links(
    FrameCoordinates to_left,
    FrameCoordinates to_right)
{
    auto new_frame = Frame(
        frame_name,
        frame_nickname,
        pattern_elements[0],
        pattern_elements[1],
        feature_set,
        feature_groups,
        to_left,
        to_right,
        accumulated_expression
    );

    return new_frame;
}

Frame Frame::with_expression(
    Expression new_expression)
{
    auto new_frame = Frame(
        frame_name,
        frame_nickname,
        pattern_elements[0],
        pattern_elements[1],
        feature_set,
        feature_groups,
        left_match,
        right_match,
        new_expression
    );

    return new_frame;
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
    return !(type_heirarchy.size() == 0) && left_match.is_empty();
}

bool Frame::is_matched()
{
    return !left_match.is_empty() && !right_match.is_empty();
}


void Frame::print_out(string title)
{
    bool is_word = is_word_frame();
    if (is_word) {
        printf("Word Frame %s:\n", title.c_str());
    }
    else {
        printf("Syntax Frame %s:\n", title.c_str());
    }

    printf("\tname: %s\n", frame_name.c_str());
    printf("\tnickname: %s\n", frame_nickname.c_str());

    if (is_word_frame()) {
        printf("\ttype heirarchy: \n\t\t");
        for(string type : type_heirarchy)
        {
            printf("%s,", type.c_str());
        }
        printf("\n");
    }
    else {
        printf("\tpattern elements: \n\t\t");
        for(PatternElement pattern_element : pattern_elements)
        {
            printf("%s,", pattern_element.match_string.c_str());
        }
        printf("\n");
    }


    printf("\tfeature set: \n\t\t");
    for(string feature : feature_set)
    {
        printf("%s,", feature.c_str());
    }
    printf("\n");

    printf("\tleft match coords:\n");
    left_match.print_out();

    printf("\tright match coords:\n");
    right_match.print_out();
}
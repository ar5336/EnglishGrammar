#ifndef DISPLAYER_HPP
#define DISPLAYER_HPP

#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include </usr/local/include/opencv4/opencv2/tracking.hpp>
#include <string>
#include <vector>

#include "../string_operators.hpp"
#include "../grammar/parser.hpp"
#include "../logic/predicate_handler.hpp"
#include "../logic/mind.hpp"

using namespace std;
using namespace cv;

Size measure_text(string text, float font_scale);

// class SchemaDisplayer
// {
// private:
//     Mat image;
    
//     ConceptualSchema *schema;

//     map<string, Point2f> noun_to_pos;
//     map<string, Size> noun_to_size;
//     vector<string> nouns;

//     float FONT_SCALE;
//     float PUSH_FACTOR;

//     Scalar CHERRY_RED;

//     bool is_connection_present(string noun_1, string noun_2);

//     void display_nouns();

//     void display_inheritances();

// public:
//     SchemaDisplayer();
//     SchemaDisplayer(ConceptualSchema *schema);

//     void drift_positions();

//     void display();
// };

class Displayer
{
private:
    Mat image;
    Point start_text_corner; // top-left position
    Point start_grid_corner;
    Point start_predicate_corner;
    Point start_individual_frame_corner;

    Point IMAGE_SIZE;

    Scalar HIGHLIGHT;
    Scalar BACKGROUND;
    Scalar GRID_BOXES; // off white
    Scalar HIGHLIGHTED_FRAME;
    Scalar WORD_TEXT_MATCHED; // teal
    Scalar WORD_TEXT_UNMATCHED; // burgundy
    Scalar WORD_FRAME; // grey sea
    Scalar SYNTAX_FRAME; // muted olive

    float PREDICATE_FONT_SCALE;
    Scalar PREDICATE_TYPE_GIVEN; // blood orange
    Scalar PREDICATE_TYPE_INFERRED; // golden orange
    Scalar PREDICATE_PARAMETER; // middling grey
    Scalar PREDICATE_ARGUMENT; // fuscia-red
    Scalar PREDICATE_COLON; // light grey
    Scalar PREDICATE_SPECIAL_ARGUMENT; // navy blue

    Scalar CONCEPTUAL_SCHEMA;

    Parser *parser;
    Mind *mind;
    PredicateHandler *predicate_handler;
    ConceptualSchema *conceptual_schema;

    int highlight_frame_index;

    // SchemaDisplayer schema_displayer;

    bool IS_INITIATED;

    string stringify_frame(Frame frame);

    string stringify_conceptual_schema_inheritances();

    string stringify_conceptual_schema_nouns();

    void display_text(Point pos, string text, Scalar color, float font_scale);

    Point2i display_multi_line_text(Point pos, string text, Scalar color, float font_scale);

    void staple_text_on(Point *pos, string text, Scalar color, float font_scale);

    void display_predicate(Point *pos, bool is_given, Predicate predicate);
public:
    string screen_name;
    int scroll;

    string response_string;

    pair<int, int> highlighted_cell_position;
    pair<int, int> previous_highlighted_cell_position;
    bool is_highlighted;

    Displayer(string screen_name);

    void init(
        Parser *parser_ptr,
        Mind* mind_ptr,
        PredicateHandler* predicate_template_handler_ptr,
        ConceptualSchema *conceptual_schema_ptr);

    void display();

    // void drift();

    bool is_in_bounds(Point point, pair<Point, Point> bounds);

    pair<Point, Point> get_cell_bounds(int row, int col);
};



#endif
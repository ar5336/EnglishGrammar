#ifndef DISPLAYER_HPP
#define DISPLAYER_HPP

#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include </usr/local/include/opencv4/opencv2/tracking.hpp>
#include <string>
#include <vector>

#include "string_operators.hpp"
// #include "frames.hpp"
#include "parser.hpp"
#include "predicate_handler.hpp"
#include "mind.hpp"

using namespace std;
using namespace cv;


class Displayer
{
private:
    Mat image;
    Point start_text_corner; // top-left position
    Point start_grid_corner;
    Point start_predicate_corner;

    Point IMAGE_SIZE;

    Scalar HIGHLIGHTER_YELLOW;
    Scalar BACKGROUND;
    Scalar GRID_BOXES; // off white
    Scalar WORD_TEXT_MATCHED; // teal
    Scalar WORD_TEXT_UNMATCHED; // burgundy
    Scalar WORD_FRAME; // grey sea
    Scalar SYNTAX_FRAME; // muted olive

    float PREDICATE_FONT_SCALE;
    Scalar PREDICATE_TYPE_GIVEN; // blood orange
    Scalar PREDICATE_TYPE_INFERRED; // golden orange
    Scalar PREDICATE_PARAMETER; // middling grey
    Scalar PREDICATE_ARGUMENT; // fuscia-red
    Scalar PREDICATE_COLON; // a light grey
    Scalar PREDICATE_SPECIAL_ARGUMENT; // a navy blue

    Parser *parser;

    Mind *mind;

    PredicateHandler *predicate_handler;

    void display_text(Point pos, string text, Scalar color, float font_scale);

    void staple_text_on(Point *pos, string text, Scalar color, float font_scale);

    Size measure_text(string text, float font_scale);

    void display_predicate(Point *pos, bool is_given, Predicate predicate);

public:
    string screen_name;
    int scroll;

    string response_string;

    Displayer(string screen_name);

    void init(Parser *parser_ptr, Mind* mind_ptr, PredicateHandler* predicate_template_handler_ptr);

    void display();

    bool is_in_bounds(Point point, pair<Point, Point> bounds);

    pair<Point, Point> get_cell_bounds(int row, int col);
};



#endif
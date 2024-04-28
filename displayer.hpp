#ifndef DISPLAYER_HPP
#define DISPLAYER_HPP

#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include </usr/local/include/opencv4/opencv2/tracking.hpp>
#include <string>
#include <vector>

#include "string_operators.hpp"
#include "frames.hpp"
#include "parser.hpp"
#include "predicate.hpp"

using namespace std;
using namespace cv;


class Displayer
{
private:
    Mat image;
    Point start_text_corner; // top-left position
    Point start_grid_corner;
    Point start_predicate_corner;

    Scalar HIGHLIGHTER_YELLOW;

    Parser *parser;

    PredicateHandler *predicate_handler;

    void display_text(Mat img, Point pos, string text, Scalar color, float font_scale);

    Size measure_text(string text, float font_scale);

public:
    string screen_name;
    int scroll;

    Displayer(string screen_name);

    void init(Parser *parser_ptr, PredicateHandler* predicate_handler_ptr);

    void display();

    bool is_in_bounds(Point point, pair<Point, Point> bounds);

    pair<Point, Point> get_cell_bounds(int row, int col);
};



#endif
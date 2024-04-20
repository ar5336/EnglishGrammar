#ifndef DISPLAYER_HPP
#define DISPLAYER_HPP

#include </usr/local/include/opencv4/opencv2/highgui.hpp>
#include </usr/local/include/opencv4/opencv2/tracking.hpp>
#include <string>
#include <vector>

#include "string_operators.hpp"
#include "frames.hpp"
#include "parser.hpp"

using namespace std;
using namespace cv;


class Displayer
{
private:
    Mat image;
    Point start_text_corner; // top-left position
    Point start_grid_corner;

    Scalar HIGHLIGHTER_YELLOW;

    Parser *parser;

    void display_text(Mat img, Point pos, string text, Scalar color, float font_scale);

    Size measure_text(string text, float font_scale);

public:
    string screen_name;

    Displayer(string screen_name);

    void init(Parser *parser_ptr, void (*mouse_callback_func)(int, int, int, int, void*));

    void display();

    bool is_in_bounds(Point point, pair<Point, Point> bounds);

    pair<Point, Point> get_cell_bounds(int row, int col);
};



#endif
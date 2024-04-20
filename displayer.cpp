#include "displayer.hpp"

bool Displayer::is_in_bounds(Point point, pair<Point, Point> bounds)
{
    Point top_left = bounds.first;
    Point bottom_right = bounds.second;

    return point.x >= top_left.x && point.x <= bottom_right.x && point.y >= top_left.y && point.y <= bottom_right.y;
}

void Displayer::display_text(Mat img, Point pos, string text, Scalar color, float font_scale = 1.0)
{
    putText(img,  // target image
            text, // text
            pos,
            cv::FONT_HERSHEY_TRIPLEX,
            font_scale,
            color, // font color
            1);
}

Size Displayer::measure_text(string text, float font_scale = 1.0)
{
    int was_found;
    return getTextSize(text,
                    cv::FONT_HERSHEY_TRIPLEX,
                    1.0,
                    2,
                    &was_found);
}

Displayer::Displayer(string screen_name)
    : screen_name(screen_name)
    {
        image = Mat(512, 1024, CV_8UC3, cv::Scalar(0));
        start_text_corner = cv::Point(10, image.rows * 3 / 4);
        start_grid_corner = start_text_corner + Point(0, -60);

        HIGHLIGHTER_YELLOW = CV_RGB(50, 25, 0);
    };

void Displayer::init(Parser *parser_ptr, void (*mouse_callback_func)(int, int, int, int, void*)) {
    // set the callback function for any mouse event

    setMouseCallback(screen_name, mouse_callback_func, NULL);

    resizeWindow(screen_name, 1024, 512);
    
    parser = parser_ptr;
}

void Displayer::display()
{
    vector<string> split_tokens;
    vector<Frame> word_frames;
    boost::split(split_tokens, parser->current_utterance, boost::is_any_of(" "), boost::token_compress_on);

    image.setTo(Scalar(0)); // clear screen

    int token_count = split_tokens.size();

    vector<bool> is_word_highlighted;

    if (!parser->parse_grid.empty() && token_count != 0)
    {
        // diplay and initialize parse grid

        //  r3  X
        //  r2  X  X
        //  r1  X  X  X
        //  r0  X  X  X  X
        //      c0 c1 c2 c3

        int cell_width = 80;
        int cell_height = 20;

        for (int row = 0; row < parser->parse_grid.size(); row++)
        {
            for (int col = 0; col < parser->parse_grid.at(row).size(); col++)
            {

                pair<Point, Point> cell_bounds = get_cell_bounds(row, col);

                Point top_left = cell_bounds.first;
                Point bottom_right = cell_bounds.second;

                bool is_covered_by_highlight;
                bool is_this_cell_selected;
                if (parser->is_highlighted)
                {
                    int highlight_row = parser->highlighted_cell_position.first;
                    int highlight_col = parser->highlighted_cell_position.second;
                    int d_row = highlight_row - row;
                    int d_col = col - highlight_col;

                    is_covered_by_highlight = (row <= highlight_row && col >= highlight_col && d_col <= d_row);
                }
                else
                {
                    is_covered_by_highlight = false;
                }

                if (row == 0)
                    is_word_highlighted.push_back(is_covered_by_highlight);

                // check if this cell is highlighted
                if (is_covered_by_highlight)
                {

                    rectangle(image, top_left, bottom_right, HIGHLIGHTER_YELLOW, cv::FILLED);
                }
                else
                {
                    rectangle(image, top_left, bottom_right, CV_RGB(0, 0, 0), cv::FILLED);
                }

                // draw rectangle
                rectangle(image, top_left, bottom_right, CV_RGB(255, 255, 255));

                vector<Frame> frames_in_cell = parser->parse_grid.at(row).at(col);

                float cell_font_scale = .5f;

                Point bottom_left = top_left + Point(3, cell_height - 3);
                Point ticker_cell_text = bottom_left;
                for (int frame_index = 0; frame_index < frames_in_cell.size(); frame_index++)
                {
                    Frame frame = frames_in_cell.at(frame_index);
                    // display word
                    string cell_text;
                    if (row == 0)
                    {
                        cell_text = frame.get_part_of_speech();
                        display_text(image, ticker_cell_text, cell_text, CV_RGB(100, 100, 200), cell_font_scale);
                    }
                    else
                    {
                        // display frame
                        cell_text = frame.frame_nickname;
                        display_text(image, ticker_cell_text, cell_text, CV_RGB(100, 200, 100), cell_font_scale);
                    }
                    ticker_cell_text += Point(measure_text(cell_text, cell_font_scale).width, 0);
                }
            }
        }
    }

    // display the text
    Point ticker_text_corner = start_text_corner;
    for (int token_index = 0; token_index < split_tokens.size(); token_index++)
    {
        string token = split_tokens[token_index];
        if (token.size() == 0)
            continue;
        
        auto word_frames = parser->parse_grid[0][token_index];
        bool does_match = !(word_frames.size() == 0);

        if (token_index != split_tokens.size() - 1)
            token += ' '; // if not last token, add a space

        if (parser->is_highlighted && is_word_highlighted[token_index])
        {
            cv::Size size = measure_text(token);
            rectangle(image, ticker_text_corner, ticker_text_corner + Point(size.width, -size.height), HIGHLIGHTER_YELLOW, cv::FILLED);
        }

        if (!does_match)
        {
            // not present
            display_text(image, ticker_text_corner, token, CV_RGB(255, 10, 10));
        }
        else
        {
            display_text(image, ticker_text_corner, token, CV_RGB(118, 185, 0)); // draw token in green
        }

        // update text_corner position
        ticker_text_corner += Point(measure_text(token).width, 0);
    }

    int total_tokenized_width = ticker_text_corner.x - start_text_corner.x;

    // display the text cursor
    Size text_size = measure_text(parser->current_utterance);

    Point cursor_top = start_text_corner + Point(total_tokenized_width, 0);
    Point cursor_bottom = cursor_top + Point(0, -text_size.height);

    cv::line(image, cursor_top, cursor_bottom, CV_RGB(200, 20, 20), 2, cv::LINE_4, 0);

    cv::imshow("reader", image);
}

pair<Point, Point> Displayer::get_cell_bounds(int row, int col)
{
    int cell_width = 80;
    int cell_height = 20;

    Point top_left = start_grid_corner + Point(col * cell_width + (row * cell_width / 2), -(row * cell_height));
    Point bottom_right = top_left + Point(cell_width, cell_height);

    return pair<Point, Point>(top_left, bottom_right);
}
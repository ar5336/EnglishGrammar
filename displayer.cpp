#include "displayer.hpp"

bool Displayer::is_in_bounds(Point point, pair<Point, Point> bounds)
{
    Point top_left = bounds.first;
    Point bottom_right = bounds.second;

    return point.x >= top_left.x && point.x <= bottom_right.x && point.y >= top_left.y && point.y <= bottom_right.y;
}

void Displayer::display_text(Point pos, string text, Scalar color, float font_scale = 1.0)
{
    putText(image,
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
                    font_scale,
                    2,
                    &was_found);
}

Displayer::Displayer(string screen_name)
    : screen_name(screen_name)
    {
        IMAGE_SIZE = Point(1800, 800);

        image = Mat(IMAGE_SIZE, CV_8UC3, cv::Scalar(0));
        start_text_corner = Point(10, image.rows * 3 / 4);
        start_grid_corner = start_text_corner + Point(0, -60);
        start_predicate_corner = Point(image.cols *6/10, image.rows * 1/4);

        HIGHLIGHTER_YELLOW = CV_RGB(50, 25, 0);
        BACKGROUND = CV_RGB(13,5,7);
        GRID_BOXES = CV_RGB(210,200,215);
        WORD_TEXT_MATCHED = CV_RGB(18,163,76);
        WORD_TEXT_UNMATCHED = CV_RGB(99,18,6);
        WORD_FRAME = CV_RGB(69, 79, 89);
        SYNTAX_FRAME = CV_RGB(70, 89, 69);

        PREDICATE_FONT_SCALE = 0.6f;
        PREDICATE_TYPE = CV_RGB(156, 96, 8);
        PREDICATE_PARAMETER = CV_RGB(66, 66, 66);
        PREDICATE_ARGUMENT = CV_RGB(125, 6, 40);
        PREDICATE_COLON = CV_RGB(117, 116, 116);
        PREDICATE_SPECIAL_ARGUMENT = CV_RGB(24, 34, 84);

        scroll = 0;
    };

void Displayer::init(
    Parser *parser_ptr,
    Mind* mind_ptr,
    PredicateHandler* predicate_handler_ptr) {
    // set the callback function for any mouse event

    

    resizeWindow(screen_name, IMAGE_SIZE);
    
    parser = parser_ptr;
    mind = mind_ptr;
    predicate_handler = predicate_handler_ptr;
}

void Displayer::display()
{
    vector<string> split_tokens = split_spaces(parser->current_utterance);
    vector<Frame> word_frames;

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
                    // rectangle(image, top_left, bottom_right, CV_RGB(0, 0, 0), cv::FILLED);
                }

                // draw rectangle
                rectangle(image, top_left, bottom_right, GRID_BOXES);

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
                        display_text(ticker_cell_text, cell_text, WORD_FRAME, cell_font_scale);
                    }
                    else
                    {
                        // display frame
                        cell_text = frame.frame_nickname;
                        display_text(ticker_cell_text, cell_text, SYNTAX_FRAME, cell_font_scale);
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
            display_text(ticker_text_corner, token, WORD_TEXT_UNMATCHED);
        }
        else
        {
            display_text(ticker_text_corner, token, WORD_TEXT_MATCHED);
        }

        // update text_corner position
        ticker_text_corner += Point(measure_text(token).width, 0);
    }

    int total_tokenized_width = ticker_text_corner.x - start_text_corner.x;

    // display the text cursor
    Size text_size = measure_text(parser->current_utterance);

    Point cursor_top = start_text_corner + Point(total_tokenized_width, 0);
    Point cursor_bottom = cursor_top + Point(0, -text_size.height);

    cv::line(image, cursor_top, cursor_bottom, WORD_TEXT_MATCHED, 2, cv::LINE_4, 0);

    // display expression handler

    Point expression_ticker_corner = start_predicate_corner + Point(0, scroll);
    Point new_line = Point(0,25);

    if (mind->expressions.size() > 0) {
        for (auto expression_of_type : mind->expressions) {
            auto expression = expression_of_type.second;
            // auto expr_type = expression_of_type.first;
            // if (expr_type == KnowledgeType::GIVEN) {
            vector<string> result_predicates = split_character(predicate_handler->stringify_expression(expression), "\n");
            
            for (Predicate predicate : expression.predicates)
            {
                auto predicate_ticker_corner = expression_ticker_corner;

                string predicate_name = predicate.predicate_template.predicate;
                // first display the predicate name
                staple_text_on(&predicate_ticker_corner, predicate_name + " ", PREDICATE_TYPE, PREDICATE_FONT_SCALE);

                vector<string> param_names = predicate.predicate_template.parameter_names;
                vector<string> pred_args = predicate.arguments;
                for (int i = 0; i < param_names.size(); i++)
                {
                    string param_name = param_names[i];
                    string pred_arg = pred_args[i];

                    staple_text_on(&predicate_ticker_corner, param_name, PREDICATE_PARAMETER, PREDICATE_FONT_SCALE);
                    staple_text_on(&predicate_ticker_corner, ":", PREDICATE_COLON, PREDICATE_FONT_SCALE);
                    if (equals(pred_arg, "unknown"))
                    {
                        staple_text_on(&predicate_ticker_corner, pred_arg + " ", PREDICATE_SPECIAL_ARGUMENT, PREDICATE_FONT_SCALE);
                    } else {
                        staple_text_on(&predicate_ticker_corner, pred_arg + " ", PREDICATE_ARGUMENT, PREDICATE_FONT_SCALE);
                    }

                }

                // display_text(expression_ticker_corner, result_predicate, PREDICATE_TYPE, 0.6f);
                expression_ticker_corner += new_line;
            }

            // } else {
            //     display_text(image, expression_ticker_corner, expression.stringify(), CV_RGB(255, 140, 0), 0.6f);

            // }
            expression_ticker_corner += new_line;
        }
    }

    // display the response string
    // Point response_corner = Point(image.cols * 8 / 10, image.rows / 8);
    Point response_corner = Point(30, 30);
    if (response_string.size() != 0) {
        display_text(response_corner, response_string, CV_RGB(255, 30, 200), 1.2f);
    }

    cv::imshow(screen_name, image);
}

void Displayer::staple_text_on(Point *pos, string text, Scalar color, float font_scale)
{
    display_text(Point(pos->x, pos->y), text, color, font_scale);
    float text_width = measure_text(text, font_scale).width;
    pos->x += text_width;

}

pair<Point, Point> Displayer::get_cell_bounds(int row, int col)
{
    int cell_width = 80;
    int cell_height = 20;

    Point top_left = start_grid_corner + Point(col * cell_width + (row * cell_width / 2), -(row * cell_height));
    Point bottom_right = top_left + Point(cell_width, cell_height);

    return pair<Point, Point>(top_left, bottom_right);
}
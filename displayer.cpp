#include "displayer.hpp"

Size measure_text(string text, float font_scale = 1.0)
{
    int was_found;
    return getTextSize(text,
                    cv::FONT_HERSHEY_TRIPLEX,
                    font_scale,
                    2,
                    &was_found);
}

// void SchemaDisplayer::display_nouns()
// {
//     vector<string> new_nouns = vector<string>();

//     for (auto noun_and_pos : noun_to_pos)
//     {
//         string noun = noun_and_pos.first;
//         new_nouns.push_back(noun);
//         Point pos = noun_and_pos.second;
//         Size size = noun_to_size.at(noun);

//         // display a black box behind the noun, the size of the noun
//         // auto text_size = Point(measure_text(noun, FONT_SCALE));

//         Point half_size = Point(size.width/2, size.height/2);
//         Point top_left = pos - half_size;
//         Point top_right = pos + half_size;

//         rectangle(image, Rect(top_left, top_right), CV_RGB(0,0,0), -1);

//         Point bottom_left = top_left + Point(0, size.height);

//         // then draw the noun
//         printf("displaying noun %s\n", noun.c_str());
//         putText(image,
//                 noun, // text
//                 bottom_left,
//                 cv::FONT_HERSHEY_TRIPLEX,
//                 FONT_SCALE,
//                 CHERRY_RED, // font color
//                 1);
//     }
    
//     nouns = new_nouns;
// }

// bool SchemaDisplayer::is_connection_present(string noun_1, string noun_2)
// {
//     auto connection_map = schema->child_to_parents_map;
//     if (connection_map.count(noun_1) != 0 &&
//         connection_map.at(noun_1).count(noun_2) != 0)
//         return true;
//     return false;
// }


// void SchemaDisplayer::drift_positions()
// {
//     map<string, Point2f> displacement_map = map<string, Point2f>();
//     for (string noun : nouns)
//     {
//         displacement_map.emplace(noun, Point(0, 0));
//     }

//     // for each noun
//     for (int i = 0; i < int(nouns.size())-1; i++)
//     {
//         // for every other noun
//         for (int j = i + 1; j < int(nouns.size()); j++)
//         {
//             // printf("hello\n");

//             if (i == j)
//                 throw runtime_error("i and j should not equal");
            
//             string noun_1 = nouns[i];
//             string noun_2 = nouns[j];
            
//             Point2f pos_1 = noun_to_pos.at(noun_1);
//             Point2f pos_2 = noun_to_pos.at(noun_2);

//             double dist = sqrt(pow(pos_1.x - pos_2.x, 2) + pow(pos_2.y - pos_2.y, 2));

//             Point2f displacement_1 = Point(0,0);
//             Point2f displacement_2 = Point(0,0);

//             Point2f from_1_to_2 = pos_2 - pos_1;
//             Point2f from_2_to_1 = pos_1 - pos_2;
//             // if they're too close (<30 units), push apart

//             if (dist < 50.0f)
//             {
//                 displacement_1 += from_2_to_1 * PUSH_FACTOR;
//                 displacement_2 += from_1_to_2 * PUSH_FACTOR;
//                 printf("displacement1 x:%f y:%f\n", displacement_1.x, displacement_1.y);
//             }

//             // if they're connected, and >50 units apart, push together

//             if (is_connection_present(noun_1, noun_2) ||
//                 is_connection_present(noun_2, noun_1))
//             {
//                 if (dist > 100.0f)
//                 {
//                     displacement_1 += from_1_to_2 * PUSH_FACTOR;
//                     displacement_2 += from_2_to_1 * PUSH_FACTOR;
//                 }
//             }

//             displacement_map.at(noun_1) += displacement_1;


//             displacement_map.at(noun_2) += displacement_2;
//         }
//     }

//     // apply displacements
//     for (string noun : nouns)
//     {
//         if (displacement_map.count(noun) != 0)
//             noun_to_pos.at(noun) += displacement_map.at(noun);
//     }
// }

// void SchemaDisplayer::display_inheritances()
// {
//     for (auto connection_pair : schema->child_to_parents_map)
//     {
//         string child = connection_pair.first;
//         set<string> parents = connection_pair.second;

//         for (string parent : parents)
//         {
//             auto child_pos = noun_to_pos.at(child);
//             auto parent_pos = noun_to_pos.at(parent);
//             line(image, child_pos, parent_pos, CHERRY_RED, LineTypes::LINE_4);
//         }
//     }
// }

// void SchemaDisplayer::display()
// {
//     // first, make sure to add any new nouns
//     image.setTo(Scalar(0, 0, 0));
//     int noun_count = 0;
//     for (string noun : schema->noun_set)
//     {
//         if (noun_to_pos.count(noun) == 0)
//         {
//             noun_to_pos.emplace(noun, Point(60 + noun_count * 4, 60 + noun_count * 4));
//             noun_to_size.emplace(noun, measure_text(noun, FONT_SCALE));
//             noun_count++;
//         }
//         nouns.push_back(noun);
//     }

//     display_inheritances();
//     drift_positions();
//     display_nouns();

//     // overlay the image over the original image

//     // Point display_position = Point(30, 30);
//     // // overlay the schema image on the original image at display_position


//     // Point other_point = Point(image.size().width, image.size().height) + display_position;
//     // Rect ROI = Rect(display_position, other_point);

//     // Mat cloned_image = original_image.clone();

//     imshow("schema", image);

//     // auto RegionOfInterest = Mat(original_image, ROI);
//     // printf("image size width:%d, height:%d\n", image.size().width, image.size().height);

//     // printf("RegionOfInterest width:%d, height:%d\n", RegionOfInterest.size().width, RegionOfInterest.size().height);

//     // image.copyTo(Mat(cloned_image, ROI));
//     // // resize(RegionOfInterest, RegionOfInterest, original_image.size());
//     // // RegionOfInterest.copyTo(original_image);
//     // return cloned_image;
// }

// SchemaDisplayer::SchemaDisplayer(ConceptualSchema *schema) : schema(schema)
// {
//     image = Mat(Point(800, 500), CV_8UC3, cv::Scalar(0));
//     PUSH_FACTOR = 0.1f;
//     CHERRY_RED = CV_RGB(200, 25, 25);
//     FONT_SCALE = 0.5f;
// }

// SchemaDisplayer::SchemaDisplayer(){}


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
        PREDICATE_TYPE_GIVEN = CV_RGB(166, 42, 8);
        PREDICATE_TYPE_INFERRED = CV_RGB(156, 96, 8);
        PREDICATE_PARAMETER = CV_RGB(66, 66, 66);
        PREDICATE_ARGUMENT = CV_RGB(125, 6, 40);
        PREDICATE_COLON = CV_RGB(117, 116, 116);
        PREDICATE_SPECIAL_ARGUMENT = CV_RGB(24, 34, 84);

        IS_INITIATED = false;

        scroll = 0;
    };

void Displayer::init(
    Parser *parser_ptr,
    Mind* mind_ptr,
    PredicateHandler* predicate_handler_ptr,
    ConceptualSchema* conceptual_schema_ptr) {
    // set the callback function for any mouse event

    resizeWindow(screen_name, IMAGE_SIZE);
    
    parser = parser_ptr;
    mind = mind_ptr;
    predicate_handler = predicate_handler_ptr;
    conceptual_schema = conceptual_schema_ptr;

    // schema_displayer = SchemaDisplayer(conceptual_schema_ptr);
    IS_INITIATED = true;
}

void Displayer::display_predicate(Point *pos, bool is_given, Predicate predicate)
{
    auto pred_color = is_given ? PREDICATE_TYPE_GIVEN : PREDICATE_TYPE_INFERRED;

    string predicate_name = predicate.predicate_template.predicate;
    // first display the predicate name
    staple_text_on(pos, predicate_name + " ", pred_color, PREDICATE_FONT_SCALE);

    vector<string> param_names = predicate.predicate_template.parameter_names;
    vector<string> pred_args = predicate.arguments;
    for (int i = 0; i < param_names.size(); i++)
    {
        string param_name = param_names[i];
        string pred_arg = pred_args[i];

        staple_text_on(pos, param_name, PREDICATE_PARAMETER, PREDICATE_FONT_SCALE);
        staple_text_on(pos, ":", PREDICATE_COLON, PREDICATE_FONT_SCALE);
        if (equals(pred_arg, "unknown"))
        {
            staple_text_on(pos, pred_arg + " ", PREDICATE_SPECIAL_ARGUMENT, PREDICATE_FONT_SCALE);
        } else {
            staple_text_on(pos, pred_arg + " ", PREDICATE_ARGUMENT, PREDICATE_FONT_SCALE);
        }

    }
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
            auto expr_type = expression_of_type.first;
            // if (expr_type == KnowledgeType::GIVEN) {
            vector<string> result_predicates = split_character(predicate_handler->stringify_expression(expression), "\n");
            
            for (Predicate predicate : expression.predicates)
            {
                auto predicate_ticker_corner = expression_ticker_corner;

                if (expr_type == KnowledgeType::GIVEN)
                    display_predicate(&predicate_ticker_corner, true, predicate);

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
    Point response_corner = Point(30, image.rows - 30);
    if (response_string.size() != 0) {
        display_text(response_corner, response_string, CV_RGB(255, 30, 200), 1.2f);
    }

    // display the conceptual schema
    // TODO - refactor this out into a ConceptualSchemaDisplayer
    auto conceptual_nouns = conceptual_schema->noun_set;

    Point conschem_corner = Point(30, 30);
    Point conschem_other_corner = conschem_corner + Point(100, 60);

    Scalar CHERRY_RED = CV_RGB(225, 25, 15);

    // for (string conceptual_noun : conceptual_nouns)
    // {
    //     noun_to_pos.emplace(conceptual_noun, conschem_ticker);
    //     staple_text_on(&conschem_ticker, conceptual_noun, CHERRY_RED, 0.5f);
    //     conschem_ticker += Point(10, 0);
    // }


    if (IS_INITIATED)
    {
        // schema_displayer.display();
    }

    cv::imshow(screen_name, image);
}

// void Displayer::drift()
// {
//     schema_displayer.drift_positions();
// }

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
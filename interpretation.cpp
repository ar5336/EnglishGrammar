#include "interpretation.hpp"

// Interpretation::Interpretation(Parser parser) {
//     // assume parser has top interpretation
    
// }

InterpretationHandler::InterpretationHandler(Parser* parser, Frame base_frame)
    : base_frame(base_frame), parser(parser) { }

void InterpretationHandler::construct_predicate()
{
    // first, just process the "mammals are animals"
    Frame left_frame;
    bool has_left_frame = parser->try_get_frame_at(base_frame.left_match, left_frame);
    Frame right_frame;
    bool has_right_frame = parser->try_get_frame_at(base_frame.right_match, right_frame);

    // base_frame.print_out("base_frame");

    // TODO - add functions to frame for this
    printf("right_frame->feature_set.count() = %d\n", right_frame.feature_set.size());

    // i.e. "horses are mammals"
    if (has_left_frame && has_right_frame &&
        equals(left_frame.frame_name, "PreIndicativePhrase")
        && right_frame.feature_set.count("MainNoun") >= 1
        && right_frame.feature_set.count("plural") >= 1)
    {
        // identify "horses" and "mammals"

        Frame left_traverser;
        // auto subgroup_frame = left_frame->left_match->left_match;
        parser->try_get_frame_at(left_frame.left_match, left_traverser);

        Frame subgroup_frame;


        // right_frame.print_out("right_frame");
        // left_frame.print_out("left_frame");
        // left_traverser.print_out("left_traverser");
    }

}
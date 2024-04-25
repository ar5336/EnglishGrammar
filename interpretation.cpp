#include "interpretation.hpp"

InterpretationHandler::InterpretationHandler(Parser* parser, Frame base_frame)
    : base_frame(base_frame), parser(parser) { }

Predicate InterpretationHandler::construct_predicate()
{
    // first, just process the "horses are animals"
    Frame left_frame;
    bool has_left_frame = parser->try_get_frame_at(base_frame.left_match, left_frame);
    Frame right_frame;
    bool has_right_frame = parser->try_get_frame_at(base_frame.right_match, right_frame);

    // TODO - add functions to frame for this

    if (!has_left_frame || !has_right_frame)
        return Predicate();

    // i.e. "horses are mammals"
    if (equals(left_frame.frame_name, "PreIndicativePhrase")
        && right_frame.feature_set.count("MainNoun") >= 1
        && right_frame.feature_set.count("plural") >= 1)
    {
        // identify subcategory (left_traverser) and category (right_traverser)

        Frame left_traverser;
        parser->try_get_frame_at(left_frame.left_match, left_traverser);


        // left_traverser.print_out("left_traverser");
        // right_frame.print_out("right_frame");

        vector<string> args;
        args.push_back(left_traverser.frame_name);
        args.push_back(right_frame.frame_name);

        return Predicate(PredicateType::IS_SUBSET_OF, args);

        // printf("resulting predicate:\n%s\n", predicate.stringify().c_str());
    }

    if (equals(right_frame.frame_name, "ModalPhrase")
        && left_frame.is_word_frame()
        && left_frame.feature_set.count("plural") != 0
        && base_frame.feature_set.count("ability") != 0)
    {
        // horses can run
        // S > N MP
        // MP => can V

        Frame right_traverser;
        parser->try_get_frame_at(right_frame.right_match, right_traverser);

        vector<string> args;
        args.push_back(left_frame.frame_name);
        args.push_back(right_traverser.frame_name);

        return Predicate(PredicateType::CAN_DO, args);
    }
    return Predicate();

}
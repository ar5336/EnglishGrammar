#include "interpretation.hpp"

InterpretationHandler::InterpretationHandler(Parser* parser, Frame base_frame)
    : base_frame(base_frame), parser(parser) { }

bool InterpretationHandler::TryConstructExpression(Expression& expression)
{
    // go thru predicate's predicate formation rules

    // Frame left_frame;
    // bool has_left_frame = parser->try_get_frame_at(base_frame.left_match, left_frame);
    // Frame right_frame;
    // bool has_right_frame = parser->try_get_frame_at(base_frame.right_match, right_frame);

    // Expression combined_expression = Expression::combine_expressions(left_frame.accumulated_expression, right_frame.accumulated_expression);

    expression = base_frame.accumulated_expression;
    return true;

    // no need to traverse the tree - the expressions should already be accumulated    
    

    // // TODO - add functions to frame for this
    // //  something like < boolframe.f_has("featureName") >

    // if (!has_left_frame || !has_right_frame)
    //     return false;

    // if (equals(base_frame.frame_name, "Question")
    //     && base_frame.feature_set.count("ability") != 0) {
    //     // i.e. "can horses run"
    //     // Q
    //     //   vp
    //     // v(ability)


    //     if (equals(right_frame.frame_name, "VerbPhrase")
    //         && left_frame.is_word_frame()
    //         && left_frame.feature_set.count("ModalVerb"))
    //     {
    //         Frame rr_f;
    //         if (!parser->try_get_frame_at(right_frame.right_match, rr_f))   
    //             return false;   

    //         Frame rl_f;
    //         if (!parser->try_get_frame_at(right_frame.left_match, rl_f))   
    //             return false;      

    //         if (!rr_f.is_word_frame() || !rl_f.is_word_frame())
    //             return false;
            
    //         auto args = {rl_f.frame_name, rr_f.frame_name};

    //         predicate = Predicate(PredicateType::CAN_DO, args, SpeechActs::QUESTION);

    //         return true;
    //     }

    // }
    // if (equals(base_frame.frame_name, "Sentence"))
    // {
    //     // i.e. "horses are mammals"
    //     if (equals(left_frame.frame_name, "PreIndicativePhrase")
    //         && right_frame.feature_set.count("MainNoun") != 0
    //         && right_frame.feature_set.count("plural") != 0)
    //     {
    //         // identify subcategory (left_traverser) and category (right_traverser)

    //         Frame left_traverser;
    //         if(!parser->try_get_frame_at(left_frame.left_match, left_traverser))
    //             return false;


    //         // left_traverser.print_out("left_traverser");
    //         // right_frame.print_out("right_frame");

    //         vector<string> args;
    //         args.push_back(left_traverser.frame_name);
    //         args.push_back(right_frame.frame_name);

    //         predicate = Predicate(PredicateType::IS_SUBSET_OF, args);
    //         return true;
    //         // printf("resulting predicate:\n%s\n", predicate.stringify().c_str());
    //     }

    //     if (equals(right_frame.frame_name, "ModalPhrase")
    //         && left_frame.is_word_frame()
    //         && left_frame.feature_set.count("Noun") != 0
    //         && base_frame.feature_set.count("ability") != 0)
    //     {
    //         // horses can run
    //         // S > N MP
    //         // MP => can V

    //         Frame right_traverser;
    //         if (!parser->try_get_frame_at(right_frame.right_match, right_traverser))
    //             return false;

    //         vector<string> args;
    //         args.push_back(left_frame.frame_name);
    //         args.push_back(right_traverser.frame_name);

    //         predicate = Predicate(PredicateType::CAN_DO, args);
    //         return true;
    //     }
    }

    // return false;
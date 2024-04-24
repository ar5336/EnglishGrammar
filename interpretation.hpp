#ifndef INTERPRETATION_HPP
#define INTERPRETATION_HPP

#include <string>

#include "frames.hpp"
#include "parser.hpp"
#include "predicate_statement.hpp"
#include "string_operators.hpp"

// an object that stores a tree of binary frames.
// this object can then be reconfigured back into complete frames (using info stored inside the binary frames)
// this interpretation can then be transformed into a predicate statement

// class Interpretation
// {
// public:
//     Interpretation *left_side;
//     Interpretation *right_side;

//     Frame frame;

//     Interpretation(Parser parser);

//     bool is_base_interpretation;

//     bool is_word_frame();
// };

class InterpretationHandler
{
private:
    bool try_get_left_link(Frame*& result_ptr);
	bool try_get_right_link(Frame*& result_ptr);

    Parser* parser;
public:
    Frame base_frame;

    InterpretationHandler(Parser* parser, Frame base_frame);

    void construct_predicate();
};

#endif
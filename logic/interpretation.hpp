#ifndef INTERPRETATION_HPP
#define INTERPRETATION_HPP

#include <string>

#include "../grammar/frames.hpp"
#include "../grammar/parser.hpp"

#include "../string_operators.hpp"

#include "predicate.hpp"

class InterpretationHandler
{
private:
    bool try_get_left_link(Frame*& result_ptr);
	bool try_get_right_link(Frame*& result_ptr);

    Parser* parser;
    PredicateHandler* handler;
public:
    Frame base_frame;

    InterpretationHandler(Parser* parser, Frame base_frame);

    bool try_construct_expression(Expression& expression);
};

#endif
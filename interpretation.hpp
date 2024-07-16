#ifndef INTERPRETATION_HPP
#define INTERPRETATION_HPP

#include <string>

#include "frames.hpp"
#include "parser.hpp"
#include "predicate.hpp"
#include "string_operators.hpp"

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

    bool TryConstructExpression(Expression& predicate);
};

#endif
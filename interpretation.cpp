#include "interpretation.hpp"

InterpretationHandler::InterpretationHandler(Parser* parser, Frame base_frame)
    : base_frame(base_frame), parser(parser) { }

bool InterpretationHandler::try_construct_expression(Expression& expression)
{
    expression = base_frame.accumulated_expression;
    return true;
}
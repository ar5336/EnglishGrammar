#include "predicate_rule_applier.hpp"

string PredicateRuleApplier::get_argument_accessor(
    RuleApplierContext context,
    vector<Frame> frames,
    PatternElementPredicateAccessor accessor)
{
    vector<Predicate> predicates_to_modify = vector<Predicate>();
    if (!try_get_predicates(context, frames, accessor, predicates_to_modify))
    {
        printf("\033[1;31mdisaster\033[0m: failed to access argument with accessor %s\n", accessor.stringify().c_str());
        return "unknown";
    }

    string accessor_paramter_name = accessor.parameter_name;


    auto original_predicate = predicates_to_modify[0];
    if (DEBUGGING)
    {
        printf("\033[1;34maccessing\033[0m parameter '%s' in predicate '%s'\n", accessor_paramter_name.c_str(), original_predicate.predicate_template.predicate.c_str());
    }

    if (predicates_to_modify.size() > 1)
        printf("\033[1;31mdisaster\033[0m: impermissibile ambiguity in argument accessor %s\n", accessor.stringify().c_str());

    string argument = original_predicate.get_argument(accessor_paramter_name);

    return argument;
}

bool PredicateRuleApplier::try_get_predicates(
    RuleApplierContext context,
    vector<Frame> frames,
    PatternElementPredicateAccessor accessor,
    vector<Predicate>& result_predicates)
{
    string frame_name = accessor.syntax_frame_name;

    result_predicates = vector<Predicate>();

    Frame identified_frame;
    bool has_identified_frame = false;
    for (auto frame : frames)
    {
        if (equals(frame.frame_name, frame_name))
        {
            if (has_identified_frame)
            {
                throw runtime_error("unresolvable ambiguity when accessing '" + accessor.stringify() + "'");
            }

            identified_frame = frame;
            has_identified_frame = true;
        }

    }

    string predicate_name = accessor.predicate_name;

    if (Expression::try_get_predicates_by_name(identified_frame.accumulated_expression, predicate_name, result_predicates))
        return true;

    if (DEBUGGING)
        printf("\033[1;31mfailed\033[0m to find match for accessor %s->%s.%s in expression : %s\n", 
        accessor.syntax_frame_name.c_str(), accessor.predicate_name.c_str(), accessor.parameter_name.c_str(),
        context.predicate_handler->stringify_expression(identified_frame.accumulated_expression).c_str());
    return false;
}

Expression PredicateRuleApplier::apply_formation_rules_on_expression(
    RuleApplierContext context,
    PredicateFormationRules formation_rule,
    Expression expression,
    vector<Frame> frames)
{
       // make sure that both left and right frame are matched

    // TODO - add check for "derived_from_monoframe" since we want to check for those
    // if (! ((left_frame.is_matched() || left_frame.is_word_frame()) &&
    //       ((right_frame.is_matched() || right_frame.is_word_frame()))))
    //       throw runtime_error("both frames should be matched during predicate formation");


    string wildcard_value = context.variable_namer->generate_name();

    vector<Predicate> predicates_to_destroy;
    // mark the designated predicate types for destruction, but don't destroy them yet
    for (string predicate_type_to_destroy : formation_rule.predicate_types_to_destroy)
    {
        vector<Predicate> predicates_marked_for_destruction = Expression::extract_predicate_types(expression, {predicate_type_to_destroy});
        for (auto predicate : predicates_marked_for_destruction)
        {
            predicates_to_destroy.push_back(predicate);
        }
    }

    // apply the modification rules before the formation rules. note - not hard decision may be changed
    // vector<Predicate> modified_predicates;
    for (int i = 0; i < formation_rule.predicate_modifiers.size(); i++)
    {
        PredicateModifier modifier = formation_rule.predicate_modifiers[i];

        PatternElementPredicateAccessor assignee = modifier.left_equal;

        ParameterCreationType operand_type = modifier.right_type;
        string operand_variable = "retrieval_failed";

        switch(operand_type)
        {
            case WILDCARD:
                operand_variable = wildcard_value;
                break;
            case STRING:
            case WORD_FRAME:
                operand_variable = modifier.right_equal_string;
                break;
            case FRAME_PREDICATE_PROPERTY:
                PatternElementPredicateAccessor operand = modifier.right_frame_predicate_property;
                operand_variable = get_argument_accessor(context, frames, operand);
                break;
        }


        expression = set_argument_accessor(context, expression, frames, assignee, operand_variable);
    }

    // TODO - create a PredicateFormationContext to keep track of variable names that have already been used
    vector<Predicate> created_predicates;
    for (int i = 0; i < formation_rule.predicate_creators.size(); i++)
    {
        PredicateCreator creator = formation_rule.predicate_creators[i];

        auto predicate_template = creator.predicate;

        int word_frame_index = 0;
        auto word_frame_accessors = creator.word_frame_accessors;

        int pattern_predicate_index = 0;
        auto pattern_predicate_acessors = creator.pattern_predicate_accessors;

        int parameter_string_index = 0;
        auto parameter_strings = creator.param_strings;

        vector<string> calculated_arguments = vector<string>();

        for (auto creation_type : creator.parameter_creation_types)
        {
            if (creation_type == ParameterCreationType::STRING)
            {
                if (DEBUGGING)
                    printf("creation type string\n");
    
                calculated_arguments.push_back(parameter_strings[parameter_string_index]);
                parameter_string_index++;
                continue;
            }
            else if (creation_type == ParameterCreationType::WORD_FRAME)
            {

                if (DEBUGGING)
                    printf("processing word frame accessors: [%s]\n", stringify_vector(word_frame_accessors).c_str());
                string word_frame_accessor = word_frame_accessors[word_frame_index];

                if (find_in_string(word_frame_accessor, "#"))
                {
                    // check if Verb#2 is present or something like that
                    auto left_and_right = split_character(word_frame_accessor, "#");
                    if (left_and_right.size() != 2)
                    {
                        throw runtime_error("place number after '#' when indicating non-first PatternElement");
                    }

                    int right_int = stoi(left_and_right[1]);
                    
                    if (right_int >= frames.size() + 1)
                        throw runtime_error("unsupported pattern element index '" + to_string(right_int) + "'");
                    else
                    {
                        calculated_arguments.push_back(frames[right_int-1].frame_name);
                        word_frame_index++;
                    }
                }
                else 
                {
                    Frame word_frame = Frame();
                    if (try_get_word_frame(context, word_frame_accessor, frames, word_frame))
                    {
                        if (DEBUGGING)
                            printf("found word frame: %s\n", word_frame.stringify_pre_binarization().c_str());
                        calculated_arguments.push_back(word_frame.frame_name);
                    } else {
                        calculated_arguments.push_back("retrieval_failed");
                    }
                    word_frame_index++;
                }
                
                if (DEBUGGING)
                    printf("done retrieving word frame\n");

                continue;
            }
            else if (creation_type == ParameterCreationType::FRAME_PREDICATE_PROPERTY)
            {
                if (DEBUGGING)
                    printf("creation type property\n");

                PatternElementPredicateAccessor pattern_argument_accessor = pattern_predicate_acessors[pattern_predicate_index];

                calculated_arguments.push_back(get_argument_accessor(context, frames, pattern_argument_accessor));
                pattern_predicate_index++;
                continue;
            }
            else if (creation_type == ParameterCreationType::WILDCARD)
            {
                if (DEBUGGING)
                    printf("creation type wildcard\n");
                
                calculated_arguments.push_back(wildcard_value);
                continue;
            }

            if (DEBUGGING)
                printf("no creation type?\n");
        }

        if (DEBUGGING)
            printf("adding created predicates\n");

        created_predicates.push_back(context.predicate_handler->construct_predicate(predicate_template.predicate, calculated_arguments));
    }

    if (DEBUGGING)
        printf("deleting slated predicates\n");
        
    // delete predicates slated for destruction
    for (auto predicate_to_destroy : predicates_to_destroy)
    {
        expression.extract_predicate(predicate_to_destroy);
    }

    if (DEBUGGING)
        printf("collating new expression\n");

    // add the created predicates to the expression
    auto collated_expression = Expression::combine_expressions(expression, Expression(created_predicates));

    if (DEBUGGING)
        printf("collated expression: %s\n", context.predicate_handler->stringify_expression(collated_expression).c_str());
    return collated_expression;
}

Expression PredicateRuleApplier::set_argument_accessor(
    RuleApplierContext context,
    Expression expression,
    vector<Frame> frames,
    PatternElementPredicateAccessor argument_accessor,
    string operand_variable)
{
    vector<Predicate> predicates_to_modify;
    if (!try_get_predicates(context, frames, argument_accessor, predicates_to_modify))
    {
        printf("\033[1;31mdisaster\033[0m: failed to access assignee of predicate modifier rule\n");
        return expression;
    }

        // // if (predicates_to_modify.size() > 1)
        // // printf("\033[1;31mdisaster\033[0m: impermissibile ambiguity in argument accessor %s\n", accessor.stringify().c_str());

    vector<Predicate> new_predicates;
    for (auto original_predicate : predicates_to_modify)
    {
        expression.extract_predicate(original_predicate);

        Predicate modified_predicate = original_predicate.with_modified_argument(argument_accessor.parameter_name, operand_variable);
        new_predicates.push_back(modified_predicate);
    }

    // no add all the
    for (auto original_expression_predicate : expression.predicates)
    {
        new_predicates.push_back(original_expression_predicate);
    }
    // create new expression with argument assigned


    return Expression(new_predicates);
}

bool PredicateRuleApplier::try_get_word_frame(RuleApplierContext context, string accessor, vector<Frame> frames, Frame &word_frame)
{

    // string framestr;
    // for (auto frame : frames)
    // {
    //     framestr += "[name:\"" + frame.frame_name +"\",";
    //     framestr += "[";
    //     for (string feature : frame.feature_set)
    //     {
    //         framestr += feature + ",";
    //     }
    //     framestr += "]";
    //     framestr += frame.stringify_pre_binarization();
    // }
    // framestr += "]";
    // printf("frames: %s\n", framestr.c_str());
    string frame_name = accessor;

    Frame identified_frame = Frame();
    bool has_identified_frame = false;
    for (auto frame : frames)
    {
        if (DEBUGGING)
            printf("checking '%s' == '%s'\n", stringify_set(frame.feature_set).c_str(), frame_name.c_str());
    
        if (frame.feature_set.count(frame_name) != 0)
        {
            if (has_identified_frame)
            {
                throw runtime_error("unresolvable ambiguity when accessing word frame '" + accessor + "'");
            }
            if (DEBUGGING)
                printf("found match in '%s',\n", frame_name.c_str());
    
            identified_frame = frame;
            has_identified_frame = true;
        }
    }

    // printf("done cycling frames\n");

    if (has_identified_frame && identified_frame.is_word_frame())
    {
        word_frame = identified_frame;
        // printf("returning word frame %s\n", word_frame.stringify_pre_binarization().c_str());
        return true;
    }

    if (DEBUGGING)
        printf("\033[1;31mfailed\033[0m to get frame for accessor: %s\n", accessor.c_str());
    word_frame = Frame();
    return false;
}

RuleApplierContext::RuleApplierContext(PredicateHandler *predicate_handler, VariableNamer *variable_namer)
    : predicate_handler(predicate_handler), variable_namer(variable_namer)
{
}

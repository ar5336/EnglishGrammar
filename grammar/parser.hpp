#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

#include "frames.hpp"
#include "grammar.hpp"
#include "../string_operators.hpp"

using namespace std;

class VariableNamer
{
private: 
    string alphabet = "abcdefghijklmnopqrstuvwxyz";
    int current_index;
    int prestige;

public:
    set<string> existing_names;

    VariableNamer();

    string generate_name();

    void reset();
};

class Parser
{
private:
    Grammar grammar;

    FrameCoordinates left_frame_coordinates;
    FrameCoordinates right_frame_coordinates;

    VariableNamer variable_namer;

    bool does_frame_have_features(
        Frame candidate_frame,
        bool is_left,
        Frame &consumer_frame);

    bool try_get_matched_frames(
        Frame left_consumer_frame,
        Frame right_consumer_frame,
        vector<Frame> &matched_frames);

    vector<Frame> find_matching_frames(vector<Frame> left_frames, vector<Frame> right_frames);

    Expression apply_formation_rules_on_expression(PredicateFormationRules formation_rule, Frame left_frame, Frame right_frame);

    Expression apply_predicate_creation_rule(PredicateCreator creator);

    string get_argument_accessor(Frame left_frame, Frame right_frame, PatternElementPredicateAccessor pattern_predicate_accessor);

    Expression set_argument_accessor(
        Expression combined_expression,
        Frame left_frame,
        Frame right_frame,
        PatternElementPredicateAccessor argument_accessor,
        string operand_variable);

    bool try_get_predicate(Frame left_frame, Frame right_frame, PatternElementPredicateAccessor accessor, Predicate& result_predicate);

public:
    // rows of columns of lists of frames
    //  r3  X
    //  r2  X  X
    //  r1  X  X  X
    //  r0  X  X  X  X
    //      c0 c1 c2 c3

    //       VP
    //   NP
    //  A   N    V
    // the dog barked
    vector<vector<vector<Frame>>> parse_grid;

    PredicateHandler* predicate_handler;

    string current_utterance;

    Parser(Grammar grammar);

    Parser();
    
    void update_parse_grid(string new_utterance);

    bool try_get_top_frame(Frame& interp_frame);

    bool try_get_frame_at(FrameCoordinates coords, Frame& result_frame);
};

#endif
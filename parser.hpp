#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

#include "frames.hpp"
#include "grammar.hpp"
#include "string_operators.hpp"

using namespace std;

class Parser
{
private:
    Grammar grammar;

    bool does_frame_have_features(
        Frame candidate_frame,
        bool is_left,
        Frame &consumer_frame);

    bool get_matched_frames(
        Frame left_consumer_frame,
        Frame right_consumer_frame,
        vector<Frame> &matched_frames);

    vector<Frame> find_matching_frames(vector<Frame> left_frames, vector<Frame> right_frames);

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

    pair<int, int> highlighted_cell_position;
    bool is_highlighted;

    Parser(Grammar grammar);

    Parser();
    
    void update_parse_grid(string current_utterance);

};

#endif
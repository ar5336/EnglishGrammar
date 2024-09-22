#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <string>
#include <vector>
#include <map>

#include "frames.hpp"
#include "variable_namer.hpp"
#include "predicate_rule_applier.hpp"
// #include "parser.hpp"

class Grammar
{
private:
    PredicateHandler* predicate_handler;
    VariableNamer* variable_namer;
    
    string stringify_monoframe_map(map<string, vector<pair<PatternElement, Frame>>> map);

    void accomodate_monoframe(Frame frame);

    void internalize_frame(Frame frame);
public:
    // map<Frame, Frame> monoframe_to_base_frame_map;
    vector<Frame> accomodated_monoframes;
    map<string, vector<Frame>> word_map;

    // for dogs, NP : N[plural]]
    // left: NP -> (N[plural]:NP[plural])
    map<string, vector<pair<PatternElement, Frame>>> monoframes_by_frame_name;
    // right:N -> (N[plural]:NP[plural])
    map<string, vector<pair<PatternElement, Frame>>> monoframes_by_pattern_element;

    // map<string, string> base_pos_to_type_map; // not used yet - good for advanced parsing efficiency
    // map<string, set<Frame>> pos_map;		  // not used yet - good for advanced parsing efficiency

    // map<string, string> syntax_nickname_to_name_map;
    vector<Frame> syntax_frames;
    map<string, Frame> syntax_name_map;

    vector<Frame> cnf_frames;
    map<string, vector<Frame>> cnf_map; // frame A > B C becomes map entry {"B C", "A"}

    map<int, Frame> cnf_frames_by_line;

    map<string, string> feature_to_feature_group;
    map<string, vector<string>> feature_group_to_features;
    set<string> feature_group_set;

    // map<int, vector<Frame>> pattern_length_map; // unused yet
    // map<set<string>, vector<Frame>> pattern_map; // not used at the moment - good for efficiency when parsing

    Grammar(PredicateHandler* predicate_handler, VariableNamer *variable_namer);

    Grammar();

    void add_to_word_map(Frame frame, string word_string);

    void binarize_grammar();
};

#endif
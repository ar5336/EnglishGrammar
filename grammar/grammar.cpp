#include "grammar.hpp"

Grammar::Grammar() { }

void Grammar::add_to_word_map(Frame frame, string word_string)
{
    if (!(word_map.find(word_string) == word_map.end()))
    {
        word_map.at(word_string).push_back(frame);
    }
    else
    {
        vector<Frame> frame_vector;
        frame_vector.push_back(frame);
        word_map.emplace(word_string, frame_vector);
    }
}

//
//
//
//
// (A)  B  (C)  D

//    1
//   0 \
//  / \ \
// A (B) C

// 0 > A B
// 1 > 0 C
//   > A C
void Grammar::binarize_grammar()
{ // first version with assumption of no optional frames - to be updated.
	
	if (DEBUGGING)
		printf("binarizing grammar\n");
	
	for (int frame_index = 0; frame_index < syntax_frames.size(); frame_index++)
	{
		vector<Frame> cnf_subframes;

		Frame frame = syntax_frames.at(frame_index);

		int pattern_length = frame.pattern_elements.size();
		int num_subframes = pattern_length - 1;

		vector<PatternElement> base_pattern = frame.pattern_elements;
		string base_frame_name = frame.frame_name;
		string base_frame_nickname = frame.frame_nickname;
		set<string> base_frame_feature_set = frame.feature_set;
		set<string> base_frame_feature_groups = frame.feature_groups;
		PredicateFormationRules base_frame_formaiton_rules = frame.predicate_formation_rules;
		int base_frame_origin_index = frame.definition_line;

		// create subframe 0 for pattern elements 0 and 1

		//      AL      - AL > 1 C
		//     / \      -  1 > A B
		//    1   \     
		//   / \   \    
		//  A   B   C

		//        AL       - AL > 1 D
		//       / \       -  1 > 2 C
		//      1   \      -  2 > A B
		//     / \   \
		//    2   \   \
		//   / \   \   \
		//  A   B   C   D

		// create subframes for subsequent elements
		for (int subframe_index = 0; subframe_index < num_subframes; subframe_index++)
		{
			set<string> feature_set = base_frame_feature_set;
			set<string> feature_groups = base_frame_feature_groups;

			string frame_name = base_frame_name;
			string frame_nickname = base_frame_nickname;

			int frame_origin_index = base_frame_origin_index;

			if (subframe_index != 0)
			{
				// a product of binarization
				
				frame_name += to_string(subframe_index);
				frame_nickname += to_string(subframe_index);
			}

			PatternElement pattern_right = base_pattern.at(base_pattern.size() - 1 - subframe_index);
			PatternElement pattern_left;
			if (subframe_index == num_subframes - 1)
			{
				pattern_left = base_pattern.at(0);
			}
			else
			{
				pattern_left.match_string = base_frame_name + to_string(subframe_index + 1);
			}
			// printf("CNF: %s > %s %s\n", frame_name.c_str(), pattern_left.match_string.c_str(), pattern_right.match_string.c_str());

			PredicateFormationRules formation_rules = base_frame_formaiton_rules;
			// if (subframe_index == 0)
			// {
			// 	formation_rules = base_frame_formaiton_rules;
			// 	// TODO - update the formation rules inheritance while binarizing
			// }

			Frame new_cnf_frame = Frame(
				frame_name,
				frame_origin_index,
				frame_nickname,
				pattern_left,
				pattern_right,
				feature_set,
				feature_groups,
				formation_rules);
			cnf_frames.push_back(new_cnf_frame);

			// add elements to cnf_map
			string match_pattern = pattern_left.match_string + " " + pattern_right.match_string;
			if (cnf_map.count(match_pattern) != 0)
			{
				cnf_map.at(match_pattern).push_back(new_cnf_frame);
			}
			else
			{
				vector<Frame> new_frame_vec;
				new_frame_vec.push_back(new_cnf_frame);
				cnf_map.emplace(match_pattern, new_frame_vec);
			}
		}
	}
}

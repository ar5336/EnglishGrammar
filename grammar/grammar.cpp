#include "grammar.hpp"


class Parser;

// Grammar::Grammar() : monoframes_by_left(map<string, vector<pair<PatternElement, Frame>>>()) {
// 	// pattern_element_map = map<string, vector<Frame>>();
// }

Grammar::Grammar(PredicateHandler *predicate_handler, VariableNamer *variable_namer)
	: predicate_handler(predicate_handler), variable_namer(variable_namer)
{
}

Grammar::Grammar()
{
	// throw runtime_error("attempting to initialize grammar without variable namer and predicate handler");
}

void Grammar::add_to_word_map(Frame frame, string word_string)
{
    if ((word_map.count(word_string) != 0))
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

void insert_frame_into_map(map<string, vector<Frame>>& map, Frame frame, string label)
{
	if (map.count(label) != 0)
	{
		map.at(label).push_back(frame);
	}
	else
	{
		vector<Frame> new_frame_vec;
		new_frame_vec.push_back(frame);
		map.emplace(label, new_frame_vec);
	}
}

void Grammar::internalize_frame(Frame frame)
{
	cnf_frames.push_back(frame);

	// add elements to cnf_ma

	string match_pattern = frame.pattern_elements[0].match_string + " " + frame.pattern_elements[1].match_string;

	insert_frame_into_map(cnf_map, frame, match_pattern);
}

void Grammar::accomodate_monoframe(Frame frame)
{
	if (DEBUGGING)
		printf("accomodating monoframe: %s\n", frame.stringify_pre_binarization().c_str());

	string left = frame.frame_name;
	
	if (frame.pattern_elements.size() != 1)
		throw runtime_error("patter element size wrong");
		
	PatternElement element = frame.pattern_elements[0];
	string right = element.match_string;

	accomodated_monoframes.push_back(frame);

	// Frame NP -> [N]
	// left:NP
	// right:N

	if (monoframes_by_frame_name.count(left) != 0)
	{
		monoframes_by_frame_name.at(left).push_back(make_pair(element, frame));
	}
	else
	{
		monoframes_by_frame_name.emplace(left, vector<pair<PatternElement, Frame>>{make_pair(element, frame)});
	}

	if (monoframes_by_pattern_element.count(right) != 0)
	{
		monoframes_by_pattern_element.at(right).push_back(make_pair(element, frame));
	}
	else
	{
		monoframes_by_pattern_element.emplace(right, vector<pair<PatternElement, Frame>>{make_pair(element, frame)});
	}
}

string Grammar::stringify_monoframe_map(map<string, vector<pair<PatternElement, Frame>>> map)
{
	string buildee;
	for (auto element : map)
	{
		buildee += element.first + ":";
		for (auto pair : element.second)
		{
			buildee += "\t" + pair.first.match_string + ":" + pair.second.frame_name + "\n";
		}
	}

	return buildee;
}

PatternElement impose_element(PatternElement basis, PatternElement overlap)
{
	return basis;
}

//      3
//     2  \
//   1  \  \
//  / \  \  \
// (A) B (C) D
// 1 > A B
// 2 > 1 C
//	 > B C
// 3 > 2 D
// 	 > B D

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

	// set<Frame> monoframes_to_accomodate;
	
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

		// add to frames to always be creating accomodations for
		if (pattern_length == 1)
		{
			accomodate_monoframe(frame);
			// monoframes_to_accomodate.insert(frame);


			continue;
		}
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

		// need to accomodate optional elmements
		//      3
		//     2  \
		//   1  \  \
		//  / \  \  \
		// (A) B (C) D
		// 1 > A B
		// 2 > 1 C
		//	 > B C
		// 3 > 2 D
		// 	 > B D

		// set<pair<int,int>> already_added_frame_patterns;

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

			PatternElement pattern_right = base_pattern[base_pattern.size() - 1 - subframe_index];
			PatternElement pattern_left;
			if (subframe_index == num_subframes - 1)
			{
				pattern_left = base_pattern.at(0);
			}
			else
			{
				pattern_left.match_string = base_frame_name + to_string(subframe_index + 1);
			}

			PredicateFormationRules formation_rules = base_frame_formaiton_rules;

			Frame new_cnf_frame = Frame(
				frame_name,
				frame_origin_index,
				frame_nickname,
				pattern_left,
				pattern_right,
				feature_set,
				feature_groups,
				formation_rules);

			if (subframe_index != 0)
				new_cnf_frame.type = FrameType::MonoFrame_Derived;
			
			internalize_frame(new_cnf_frame);
		}
	}

	if (DEBUGGING)
	{
		printf("monoframes_by_left: %s\n", stringify_monoframe_map(monoframes_by_frame_name).c_str());
		printf("monoframes_by_rihgt: %s\n", stringify_monoframe_map(monoframes_by_pattern_element).c_str());

		printf("done binarizing grammar\n");
	}
}

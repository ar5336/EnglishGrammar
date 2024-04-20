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

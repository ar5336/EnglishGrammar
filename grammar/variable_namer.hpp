#ifndef VARIABLE_NAMER_HPP
#define VARIABLE_NAMER_HPP

#include <string>
#include <set>

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

#endif
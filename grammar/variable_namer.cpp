#include "variable_namer.hpp"

VariableNamer::VariableNamer() {
    current_index = 0;
    prestige = 0;
    existing_names = set<string>();
}

string VariableNamer::generate_name()
{
    if (current_index < 25)
    {
        current_index++;
    }
    else{
        prestige++;
        current_index = 0;
    }

    char result = alphabet[current_index];

    string result_string = string(1,  result); 

    if (prestige >= 1){
        return result_string + to_string(prestige);
    } else {
        return result_string;
    }

    // throw runtime_error("failed to generate name");
}

void VariableNamer::reset()
{
    current_index = 0;
}
#include "string_operators.hpp"

void ltrim(string &s)
{
	// trim the space characters from the left side of the string
	if (s.size() == 0)
		return;
	if (DEBUGGING)
		printf("ltrimming string: '%s'\n", s.c_str());
	while (s.size() > 0 && isspace(s[0]))
	{
		s.erase(0, 1);
	}
}

void rtrim(string &s)
{
	// trim the space characters from the right side of the string
	if (s.size() == 0)
		return;
	
	if (DEBUGGING)
		printf("rtrimming string: '%s'\n", s.c_str());
	while (s.size() > 0 && isspace(s[s.size() - 1]))
	{
		s.erase(s.size() -1, s.size());
	}
	
}

bool starts_and_ends_with(string l, string r)
{
	// TODO - refactor
	return false;
}

string trim_front_and_back(string s)
{
	if (s.size() <= 2)
		throw runtime_error("string too small in trim_front_and_back");

	return s.substr(1, s.size() - 2);
}

void trim(string &s)
{
	rtrim(s);
	ltrim(s);
}

int count_initial_spaces(string str)
{
	if (str.size() == 0)
		return -1;
		
	char initial_char = str.at(0);
	int char_index = 0;
	while (initial_char == ' ' && char_index < str.size() - 1)
	{
		char_index++;
		initial_char = str.at(char_index);
	}
	return char_index - 1;
}

bool equals(string a, string b)
{
	return a.compare(b) == 0;
}

bool is_str_empty(string str)
{
	trim(str);
	return str.size() == 0;
}

vector<string> split_character(string str, string split)
{
	vector<string> split_tokens = vector<string>();
	if (DEBUGGING)
		printf("splitting string: '%s' by delimiter '%s'\n", str.c_str(), split.c_str());
	
	// split string by the split string
	int pos = 0;
	bool append_final = true;
	while (str.size() > 0 && (pos = str.find(split)) != string::npos)
	{
		// make sure to handle the case of multiple delimiters in a row such as "apples   oranger" with delimiter " "
		if (pos == 0)
		{
			str.erase(0, split.length());
			continue;
		}
		split_tokens.push_back(str.substr(0, pos));
		str.erase(0, pos + split.length());
	}
	if (!equals(str, split) && !is_str_empty(str))
		split_tokens.push_back(str);

	if (DEBUGGING)
		printf("returning %ld split tokens\n", split_tokens.size());
	
	return split_tokens;
}

vector<string> split_spaces(string str)
{
	return split_character(str, " ");
}

bool find_in_string(string field, string target)
{
	return field.find(target) != string::npos;
}

string remove_char_from_string(string str, char chr)
{
	string result = "";
	for (char ch : str)
	{
		if (ch != chr)
			result += ch;
	}
	return result;
}

bool is_string_all_chars(string test, char subject)
{
    for (char ch : test)
	{
		if (subject != ch)
			return false;
	}
	return true;
}

string stringify_set(set<string> set)
{
    string feature_string = "";
    for (string string : set)
    {
        feature_string += string;
        feature_string += ", ";
    }

    if (set.size() > 0)
        return feature_string.substr(0, feature_string.length()-2);
    return feature_string;
}

string stringify_vector(vector<string> vector)
{
    string feature_string = "";
    for (string string : vector)
    {
        feature_string += string;
        feature_string += ", ";
    }

    if (vector.size() > 0)
        return feature_string.substr(0, feature_string.length()-2);
    return feature_string;
}

string stringify_stoi_map(map<string, int> map)
{
	string result = "";
	for (auto element : map)
	{
		result += "[ " + element.first + " -> " + to_string(element.second) + " ]\n";
	}

    return result;
}

string stringify_bool_vec(vector<bool> vec)
{
	string bools_string = "";
    for (bool val : vec)
    {
        bools_string += val ? "true" : "false";
        bools_string += ", ";
    }

    if (vec.size() > 0)
        return bools_string.substr(0, bools_string.length()-2);
    return bools_string;
}
#include "string_operators.hpp"

void ltrim(string &s)
{
	s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch)
							   { return !isspace(ch); }));
}

// trim from end (in place)
void rtrim(string &s)
{
	s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch)
					{ return !isspace(ch); })
				.base(),
			s.end());
}

bool starts_and_ends_with(string l, string r)
{
	// TODO - refactor
	return false;
}

string trim_front_and_back(string s)
{
	if (s.size() <= 2)
		throw invalid_argument("string too small in trim_front_and_back");

	return s.substr(1, s.size() - 2);
}

void trim(string &s)
{
	rtrim(s);
	ltrim(s);
}

int count_initial_spaces(string str)
{
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
	vector<string> split_tokens;
	boost::split(split_tokens, str, boost::is_any_of(split), boost::token_compress_on);
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

bool is_string_all_chars(string test, char subject)
{
    for (char ch : test)
	{
		if (subject != ch)
			return false;
	}
	return true;
}

#ifndef STRING_OPS_HPP
#define STRING_OPS_HPP

#include <string>
#include <set>
#include <map>
#include <stdexcept>
#include <vector>

#include "global.hpp"
// #include <boost/algorithm/string.hpp>

using namespace std;

void ltrim(string &s);

void rtrim(string &s);

bool starts_and_ends_with(string l, string r);

string trim_front_and_back(string s);

void trim(string &s);

int count_initial_spaces(string str);

bool equals(string a, string b);

vector<string> split_character(string str, string split);

vector<string> split_spaces(string str);

bool find_in_string(string field, string target);

string remove_char_from_string(string str, char chr);

bool is_string_all_chars(string test, char subject);

string stringify_set(set<string> set);

string stringify_vector(vector<string> vector);

string stringify_stoi_map(map<string,int> map);

string stringify_bool_vec(vector<bool> vec);

#endif
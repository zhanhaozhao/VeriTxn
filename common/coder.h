//
// Created by pan on 2022/9/28.
//
#include "string"
#include "vector"

using namespace std;

typedef pair<string, string> encoded_record;

string encode_vec(const vector<pair<string,string> > &data);
vector<pair<string, string> > decode_vec(const string &e);

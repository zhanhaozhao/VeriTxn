//
// Created by pan on 2022/9/28.
//

#include "coder.h"

using namespace std;

DFlow encode_vec(const vector<pair<string,string> > &data) {
    string result;
    for (const auto& t:data) {
        result.append("{" + t.first + "}{" + t.second + "}");
    }
    if (!result.empty()) {
    }
    return result;
}

vector<pair<string, string> > decode_vec(const string &e) {
    vector <pair<string, string>> vector;
    int n = int(e.size()), depth = 0;
    if (n == 0) {
        return vector;
    }
    assert(n > 1);
    assert(e[0] == '{' && e[n - 1] == '}');
    bool is_first = true;
    string tmp, fi;
    for (int i = 0; i < n; i++) {
        if (depth > 1 || (depth == 1 && e[i] != '}')) {
            tmp.push_back(e[i]);
        }
        if (depth == 1 && e[i] == '}') {
            if (is_first) {
                fi = tmp;
            } else {
                vector.emplace_back(make_pair(fi, tmp));
                fi.clear();
            }
            is_first ^= 1;
            tmp.clear();
        }
        if (e[i] == '{') {
            depth++;
        } else if (e[i] == '}') {
            depth--;
        }
        if (depth == 0) {
            assert(e[i] == '}' || e[i] == '{');
        }
    }
    assert(depth == 0);
    assert(fi.empty());
    return vector;
}
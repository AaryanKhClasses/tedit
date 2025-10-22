#pragma once
#include <string>
#include <vector>
#include <map>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

struct Language {
    string name;
    vector<string> extensions;
    vector<string> keywords;
    string singleLineComments;
};

struct Theme {
    string name;
    map<string, string> colors;
};

class Syntax {
    public:
        static Language currentLanguage;
        static Theme currentTheme;
        static void loadLanguage(const string& filename);
        static void loadTheme(const string& filename);
        static void updateSyntax(vector<string>& rows, vector<vector<int>>& hl, int row);
};

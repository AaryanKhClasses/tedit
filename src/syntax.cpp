#include "syntax.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <cstdlib>
using namespace std;
namespace fs = std::filesystem;

Language Syntax::currentLanguage;
Theme Syntax::currentTheme;

static fs::path s_exeDir;
static fs::path resolveSubdir(const string& subdir) {
    vector<fs::path> bases;

    if(const char* envp = std::getenv("TEDIT_DATA_DIR")) {
        if(*envp) bases.emplace_back(fs::path(envp));
    }

    if(!s_exeDir.empty()) {
        bases.push_back(s_exeDir);
        bases.push_back(s_exeDir.parent_path());
        bases.push_back(s_exeDir.parent_path() / "share" / "tedit");
    }

    bases.push_back(fs::current_path());

    for(const auto& base : bases) {
        fs::path candidate = base / subdir;
        std::error_code ec;
        if(fs::exists(candidate, ec) && fs::is_directory(candidate, ec)) return candidate;
    }
    return fs::path(subdir);
}

void Syntax::setExecutablePath(const std::string& argv0) {
    std::error_code ec;
    fs::path p(argv0);
    if(p.is_relative()) p = fs::current_path() / p;
    p = fs::weakly_canonical(p, ec);
    if(ec) s_exeDir = p.parent_path();
    else s_exeDir = p.parent_path();
}

void Syntax::loadLanguage(const string& filename) {
    string ext = filename.substr(filename.find_last_of('.'));
    currentLanguage = {};

    fs::path langDir = resolveSubdir("languages");
    std::error_code ec;
    if(!fs::exists(langDir, ec) || !fs::is_directory(langDir, ec)) return;
    for(auto& entry : fs::directory_iterator(langDir, ec)) {
        if(ec) break;
        if(entry.is_regular_file()) {
            ifstream file(entry.path());
            if(!file) continue;
            json data;
            file >> data;

            for(auto& e : data["extensions"]) {
                if(ext == e.get<string>()) {
                    currentLanguage.name = data["name"];
                    currentLanguage.extensions = data["extensions"].get<vector<string>>();
                    currentLanguage.keywords = data["keywords"].get<vector<string>>();
                    currentLanguage.singleLineComments = data["singleLineComments"].get<string>();
                    return;
                }
            }
        }
    }
}

void Syntax::loadTheme(const string& filename) {
    fs::path themesDir = resolveSubdir("themes");
    fs::path path = themesDir / (filename + ".json");
    ifstream file(path);
    if(!file) return;
    json data;
    file >> data;
    currentTheme.name = data["name"];
    currentTheme.colors = data["colors"].get<map<string, string>>();
}

void Syntax::updateSyntax(vector<string>& rows, vector<vector<int>>& hl, int row) {
    if(row >= (int)rows.size()) return;
    if((int)hl.size() <= row) hl.resize(row + 1);
    string& line = rows[row];
    hl[row].assign(line.size(), 0);

    auto color = [&](const string& type) -> string {
        auto it = currentTheme.colors.find(type);
        if(it != currentTheme.colors.end()) return "\x1b[" + it->second + "m";
        return "\x1b[39m";
    };

    for(auto& kw : currentLanguage.keywords) {
        size_t pos = line.find(kw);
        while(pos != string::npos) {
            if((pos == 0 || !isalnum(line[pos - 1])) &&
                (pos + kw.size() == line.size() || !isalnum(line[pos + kw.size()]))) {
                for(size_t i = pos; i < pos + kw.size(); i++) hl[row][i] = 1;        
            }
            pos = line.find(kw, pos + 1);
        }
    }

    for(size_t i = 0; i < line.size(); i++) {
        if(isdigit(line[i])) hl[row][i] = 2;
    }

    bool inString = false;
    for(size_t i = 0; i < line.size(); i++) {
        if(line[i] == '"') {
            hl[row][i] = 3;
            inString = !inString;
        } else if(inString) hl[row][i] = 3;
    }

    string commentStart = currentLanguage.singleLineComments;
    size_t commentPos = line.find(commentStart);
    if(commentPos != string::npos) {
        for(size_t i = commentPos; i < line.size(); i++) hl[row][i] = 4;
    }
}

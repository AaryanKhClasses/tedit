#pragma once
#include "syntax.h"
#include <string>
#include <vector>
#include <chrono>
using namespace std;

enum EditorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

class Editor {
    public:
        Editor();
        bool processKeypress();
        void refreshScreen();
        void openFile(const string& name);
        void saveFile();
        void saveFileAs();
        
    private:
        void drawRows();
        void drawContentRows(int numRows);
        void insertChar(char ch);
        int getIndentLevel(const string& line);
        void scroll();
        void drawStatusBar();
        void setStatusMessage(const string& msg);
        int readKey();
        string promptForInput(const string& prompt);

        int cursorX, cursorY;
        int rowOffset = 0, colOffset = 0, screenRows = 24, screenCols = 80;
        vector<string> rows;

        string fileName = "[No Name]";
        string statusMessage;
        chrono::steady_clock::time_point statusTime;

        vector<vector<int>> hl;
};

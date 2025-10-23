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
        void renameFile();
        
    private:
        // Undo/Redo support
        enum class ActionType { InsertText, DeleteRange, SplitLine, JoinLine };
        struct Action {
            ActionType type;
            int y, x;                 // position reference
            string text;              // inserted text or deleted text or suffix (for split)
            string aux;               // auxiliary: indent for split, or unused
            int beforeX = 0, beforeY = 0; // cursor before action
            int afterX = 0, afterY = 0;   // cursor after action
        };

        void pushAction(const Action& a);
        void applyForward(const Action& a);
        void applyInverse(const Action& a);
        void undo();
        void redo();

        // low-level text ops (no history recording)
        void insertTextAt(int y, int x, const string& s);
        void deleteRangeAt(int y, int x, int len);

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

        vector<Action> undoStack;
        vector<Action> redoStack;
};

#include "editor.h"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
using namespace std;

Editor::Editor() : cursorX(0), cursorY(0) {
    rows.push_back("");
}

void Editor::processKeypress() {
    int key = readKey();

    switch(key) {
        case 17: // Ctrl-Q
            cout << "\x1b[2J\x1b[H"; // Clear screen
            exit(0);
            break;
        case 19: // Ctrl-S
            saveFile();
            break;
        case '\r': { // Enter Key
            string newLine = "";
            if(cursorX < (int)rows[cursorY].size()) {
                newLine = rows[cursorY].substr(cursorX);
                rows[cursorY] = rows[cursorY].substr(0, cursorX);
            }
            rows.insert(rows.begin() + cursorY + 1, newLine);
            cursorY++;
            cursorX = 0;
            break;
        } case 127: // Backspace
        case 8:   // Ctrl-H
            if(cursorX > 0) {
                rows[cursorY].erase(cursorX - 1, 1);
                cursorX--;
            } else if(cursorY > 0) {
                cursorX = rows[cursorY - 1].size();
                rows[cursorY - 1] += rows[cursorY];
                rows.erase(rows.begin() + cursorY);
                cursorY--;
            }
            break;
        case ARROW_UP:
            if(cursorY > 0) cursorY--;
            if(cursorX > (int)rows[cursorY].size()) cursorX = rows[cursorY].size();
            break;
        case ARROW_DOWN:
            if(cursorY < (int)rows.size() - 1) cursorY++;
            if(cursorX > (int)rows[cursorY].size()) cursorX = rows[cursorY].size();
            break;
        case ARROW_LEFT:
            if(cursorX > 0) cursorX--;
            else if(cursorY > 0) {
                cursorY--;
                cursorX = rows[cursorY].size();
            }
            break;
        case ARROW_RIGHT:
            if(cursorX < (int)rows[cursorY].size()) cursorX++;
            else if(cursorY < (int)rows.size() - 1) {
                cursorY++;
                cursorX = 0;
            }
            break;
        default:
            if(isprint(key)) insertChar(key);
            break;
    }

    if(key != 19) setStatusMessage("");
}

void Editor::insertChar(char ch) {
    if(cursorY >= (int)rows.size()) return;
    if(cursorX > (int)rows[cursorY].size()) cursorX = rows[cursorY].size();
    rows[cursorY].insert(rows[cursorY].begin() + cursorX, ch);
    cursorX++;
}

void Editor::refreshScreen() {
    scroll();
    cout << "\x1b[2J\x1b[H"; // Clear screen and move cursor to top-left
    drawRows();

    cout << "\x1b[" << (cursorY - rowOffset + 1) << ";" << (cursorX - colOffset + 1) << "H"; // Move cursor to (cursorY, cursorX)
    cout << "\x1b[?25h"; // Show cursor
    cout.flush();
}

void Editor::drawRows() {
    for(int y = 0; y < screenRows; y++) {
        int fileRow = y + rowOffset;
        if(fileRow >= (int)rows.size()) {
            if(y == screenRows - 1) drawStatusBar();
            else cout << "~\r\n"; // Empty Line Indicator
        }
        else {
            Syntax::updateSyntax(rows, hl, fileRow);
            string& line = rows[fileRow];

            int len = line.size() > colOffset ? line.size() - colOffset : 0;
            int drawLen = len < screenCols ? len : screenCols;

            for(int i = 0; i < drawLen; i++) {
                int hlType = 0;
                if((int)hl[fileRow].size() > i + colOffset) hlType = hl[fileRow][i + colOffset];
                switch(hlType) {
                    case 0: cout << "\x1b[39m"; break;
                    case 1: cout << "\x1b[" + Syntax::currentTheme.colors["keyword"] + "m"; break;
                    case 2: cout << "\x1b[" + Syntax::currentTheme.colors["number"] + "m"; break;
                    case 3: cout << "\x1b[" + Syntax::currentTheme.colors["string"] + "m"; break;
                    case 4: cout << "\x1b[" + Syntax::currentTheme.colors["comment"] + "m"; break;
                }
                cout << line[i + colOffset];
            }

            cout << "\x1b[39m"; // Reset to normal color
            cout << "\x1b[K\r\n"; // Clear line after content
        }
    }
}

int Editor::readKey() {
    char ch;
    if(read(STDIN_FILENO, &ch, 1) == -1) return -1;
    
    if(ch == '\x1b') {
        char seq[3];
        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if(seq[0] == '[') {
            switch(seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
        return '\x1b';
    }
    return ch;
}

void Editor::scroll() {
    if(cursorY < rowOffset) rowOffset = cursorY;
    if(cursorY >= rowOffset + screenRows) rowOffset = cursorY - screenRows + 1;
    if(cursorX < colOffset) colOffset = cursorX;
    if(cursorX >= colOffset + screenCols) colOffset = cursorX - screenCols + 1;
}

void Editor::setStatusMessage(const string& msg) {
    statusMessage = msg;
    statusTime = chrono::steady_clock::now();
}

void Editor::drawStatusBar() {
    string status;
    if(!statusMessage.empty()) {
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - statusTime).count();
        if(elapsed > 2) statusMessage = "";
        else status = statusMessage;
    }

    if(status.empty()) status = fileName + " | " + to_string(cursorY + 1) + ":" + to_string(cursorX + 1);
    if((int)status.size() > screenCols) status = status.substr(0, screenCols);

    // Invert colors for status bar
    cout << "\x1b[7m";
    cout << status;
    for(int i = status.size(); i < screenCols; i++) cout << " ";
    cout << "\x1b[m\r\n"; // Reset formatting
}

void Editor::openFile(const string& name) {
    fileName = name;
    rows.clear();

    ifstream file(name);
    if(!file) {
        setStatusMessage("Could not open file!");
        return;
    }

    string line;
    while(getline(file, line)) {
        if(!line.empty() && line.back() == '\r') line.pop_back(); // Handle CRLF
        rows.push_back(line);
    }

    if(rows.empty()) rows.push_back("");

    Syntax::loadLanguage(fileName);
    Syntax::loadTheme("default");
    hl.assign(rows.size(), vector<int>());
    for(int i = 0; i < (int)rows.size(); i++) Syntax::updateSyntax(rows, hl, i);

    setStatusMessage("File loaded successfully.");
}

void Editor::saveFile() {
    if(fileName == "[No Name]") {
        setStatusMessage("Save aborted: No file name.");
        return;
    }
    ofstream file(fileName);
    if(!file) {
        setStatusMessage("Could not save file!");
        return;
    }
    for(const auto& line : rows) {
        file << line << "\n";
    }
    setStatusMessage("File saved successfully.");
}

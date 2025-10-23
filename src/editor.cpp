#include "editor.h"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
using namespace std;

Editor::Editor() : cursorX(0), cursorY(0) {
    rows.push_back("");
}

bool Editor::processKeypress() {
    int key = readKey();

    switch(key) {
        case 17: // Ctrl-Q
            cout << "\x1b[2J\x1b[H"; // Clear screen
            return false;
            break;
        case 19: // Ctrl-S
            saveFile();
            break;
        case 23: // Ctrl-W
            saveFileAs();
            break;
        case 18: // Ctrl-R
            renameFile();
            break;
        case '\t': { // Tab Key => Insert 4 spaces as one action
            Action a;
            a.type = ActionType::InsertText;
            a.beforeX = cursorX; a.beforeY = cursorY;
            a.y = cursorY; a.x = cursorX; a.text = string(4, ' ');
            applyForward(a);
            a.afterX = cursorX; a.afterY = cursorY;
            pushAction(a);
            break;
        }
        case '\r': { // Enter Key
            // Split line at cursor; store suffix and indent in action
            Action a;
            a.type = ActionType::SplitLine;
            a.beforeX = cursorX; a.beforeY = cursorY;
            a.y = cursorY; a.x = cursorX;
            string suffix = "";
            if(cursorX < (int)rows[cursorY].size()) suffix = rows[cursorY].substr(cursorX);
            a.text = suffix; // original suffix
            a.aux = string(getIndentLevel(rows[cursorY]), ' '); // indent to apply on new line
            applyForward(a);
            a.afterX = cursorX; a.afterY = cursorY;
            pushAction(a);
            break;
        } case 127: // Backspace
        case 8:   // Ctrl-H
            if(cursorX > 0) {
                // Delete previous character as one action
                Action a;
                a.type = ActionType::DeleteRange;
                a.beforeX = cursorX; a.beforeY = cursorY;
                a.y = cursorY; a.x = cursorX - 1;
                a.text = string(1, rows[cursorY][cursorX - 1]);
                applyForward(a);
                a.afterX = cursorX; a.afterY = cursorY;
                pushAction(a);
            } else if(cursorY > 0) {
                // Join with previous line
                Action a;
                a.type = ActionType::JoinLine;
                a.beforeX = cursorX; a.beforeY = cursorY;
                a.y = cursorY; // current line index to be joined into y-1
                a.x = rows[cursorY - 1].size(); // join point in previous line
                a.text = rows[cursorY]; // content of current line
                applyForward(a);
                a.afterX = cursorX; a.afterY = cursorY;
                pushAction(a);
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
        case 26: // Ctrl-Z Undo
            undo();
            break;
        case 25: // Ctrl-Y Redo
            redo();
            break;
        default:
            if(isprint(key)) {
                Action a;
                a.type = ActionType::InsertText;
                a.beforeX = cursorX; a.beforeY = cursorY;
                a.y = cursorY; a.x = cursorX; a.text = string(1, (char)key);
                applyForward(a);
                a.afterX = cursorX; a.afterY = cursorY;
                pushAction(a);
            }
            break;
    }

    if(key != 19 && key != 23 && key != 18 && key != 26 && key != 25) setStatusMessage("");
    return true;
}

void Editor::insertChar(char ch) {
    if(cursorY >= (int)rows.size()) return;
    if(cursorX > (int)rows[cursorY].size()) cursorX = rows[cursorY].size();
    rows[cursorY].insert(rows[cursorY].begin() + cursorX, ch);
    cursorX++;
}
void Editor::insertTextAt(int y, int x, const string& s) {
    if(y < 0) return;
    if(y >= (int)rows.size()) rows.resize(y + 1);
    string& line = rows[y];
    if(x < 0) x = 0;
    if(x > (int)line.size()) x = line.size();
    line.insert(x, s);
}

void Editor::deleteRangeAt(int y, int x, int len) {
    if(y < 0 || y >= (int)rows.size()) return;
    string& line = rows[y];
    if(x < 0) x = 0;
    if(x > (int)line.size()) x = line.size();
    if(len < 0) len = 0;
    if(x + len > (int)line.size()) len = line.size() - x;
    line.erase(x, len);
}

void Editor::pushAction(const Action& a) {
    undoStack.push_back(a);
    redoStack.clear();
}

void Editor::applyForward(const Action& a) {
    switch(a.type) {
        case ActionType::InsertText: {
            insertTextAt(a.y, a.x, a.text);
            cursorY = a.y;
            cursorX = a.x + (int)a.text.size();
            break;
        }
        case ActionType::DeleteRange: {
            deleteRangeAt(a.y, a.x, (int)a.text.size());
            cursorY = a.y;
            cursorX = a.x;
            break;
        }
        case ActionType::SplitLine: {
            // rows[y] = prefix; insert rows[y+1] = indent + suffix
            string prefix = rows[a.y].substr(0, a.x);
            rows[a.y] = prefix;
            rows.insert(rows.begin() + a.y + 1, a.aux + a.text);
            cursorY = a.y + 1;
            cursorX = (int)a.aux.size();
            break;
        }
        case ActionType::JoinLine: {
            // rows[y-1] += rows[y]; remove rows[y]
            if(a.y - 1 >= 0 && a.y < (int)rows.size()) {
                rows[a.y - 1] += rows[a.y];
                rows.erase(rows.begin() + a.y);
                cursorY = a.y - 1;
                cursorX = a.x;
            }
            break;
        }
    }
}

void Editor::applyInverse(const Action& a) {
    switch(a.type) {
        case ActionType::InsertText: {
            // inverse is delete the inserted text
            deleteRangeAt(a.y, a.x, (int)a.text.size());
            cursorY = a.beforeY;
            cursorX = a.beforeX;
            break;
        }
        case ActionType::DeleteRange: {
            // inverse is re-insert the deleted text
            insertTextAt(a.y, a.x, a.text);
            cursorY = a.beforeY;
            cursorX = a.beforeX;
            break;
        }
        case ActionType::SplitLine: {
            // inverse merges back original suffix and removes inserted line
            if(a.y + 1 < (int)rows.size()) {
                rows[a.y] += a.text;
                rows.erase(rows.begin() + a.y + 1);
                cursorY = a.beforeY;
                cursorX = a.beforeX;
            }
            break;
        }
        case ActionType::JoinLine: {
            // inverse splits line back
            if(a.y - 1 >= 0 && a.y - 1 < (int)rows.size()) {
                string& prev = rows[a.y - 1];
                string suffix = prev.substr(a.x);
                prev.erase(a.x);
                rows.insert(rows.begin() + a.y, suffix);
                cursorY = a.beforeY;
                cursorX = a.beforeX;
            }
            break;
        }
    }
}

void Editor::undo() {
    if(undoStack.empty()) { setStatusMessage("Nothing to undo"); return; }
    Action a = undoStack.back(); undoStack.pop_back();
    applyInverse(a);
    redoStack.push_back(a);
}

void Editor::redo() {
    if(redoStack.empty()) { setStatusMessage("Nothing to redo"); return; }
    Action a = redoStack.back(); redoStack.pop_back();
    applyForward(a);
    undoStack.push_back(a);
}

int Editor::getIndentLevel(const string& line) {
    int indent = 0;
    for(char ch : line) {
        if(ch == ' ') indent++;
        else if(ch == '\t') indent += 4;
        else break;
    }
    return indent;
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
    drawContentRows(screenRows);
}

void Editor::drawContentRows(int numRows) {
    for(int y = 0; y < numRows; y++) {
        int fileRow = y + rowOffset;
        if(fileRow >= (int)rows.size()) {
            if(y == numRows - 1 && numRows == screenRows) drawStatusBar();
            else {
                cout << "~";
                if(y < numRows - 1) cout << "\r\n";
            }
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
            cout << "\x1b[K"; // Clear line after content
            if(y < numRows - 1) cout << "\r\n";
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
    for(const auto& line : rows) file << line << "\n";
    setStatusMessage("File saved successfully.");
}

void Editor::saveFileAs() {
    string newFileName = promptForInput("Save as: ");
    if(newFileName.empty()) {
        setStatusMessage("Save as aborted.");
        return;
    }

    fileName = newFileName;
    ofstream file(fileName);
    if(!file) {
        setStatusMessage("Could not save file!");
        return;
    }
    for(const auto& line : rows) file << line << "\n";

    Syntax::loadLanguage(fileName);
    Syntax::loadTheme("default");
    hl.assign(rows.size(), vector<int>());
    for(int i = 0; i < (int)rows.size(); i++) Syntax::updateSyntax(rows, hl, i);

    setStatusMessage("File saved as " + fileName);
}

void Editor::renameFile() {
    string newName = promptForInput("Rename to: ");
    if(newName.empty()) {
        setStatusMessage("Rename aborted.");
        return;
    }

    if(newName == fileName) {
        setStatusMessage("Same name. Nothing to do.");
        return;
    }

    if(fileName == "[No Name]") {
        // Treat as save-as for an unnamed buffer
        fileName = newName;
        ofstream out(fileName);
        if(!out) {
            setStatusMessage("Could not create file!");
            fileName = "[No Name]";
            return;
        }
        for(const auto& line : rows) out << line << "\n";
    } else {
        // Ensure current contents are written, then rename the file on disk
        saveFile();
        std::error_code ec;
        std::filesystem::rename(fileName, newName, ec);
        if(ec) {
            // Fallback: write to new file and remove the old one
            ofstream out(newName);
            if(!out) {
                setStatusMessage("Rename failed: cannot write new file.");
                return;
            }
            for(const auto& line : rows) out << line << "\n";
            std::filesystem::remove(fileName, ec); // ignore error
        }
        fileName = newName;
    }

    // Reload syntax highlighting for the new file extension
    Syntax::loadLanguage(fileName);
    Syntax::loadTheme("default");
    hl.assign(rows.size(), vector<int>());
    for(int i = 0; i < (int)rows.size(); i++) Syntax::updateSyntax(rows, hl, i);

    setStatusMessage("Renamed to " + fileName);
}

string Editor::promptForInput(const string& prompt) {
    string input = "";

    while(true) {
        // Clear screen and draw content rows (excluding status bar)
        cout << "\x1b[2J\x1b[H";
        drawContentRows(screenRows - 1);
        
        cout << "\r\n\x1b[K"; // New line and clear line
        cout << "\x1b[7m"; // Invert colors
        string status = prompt + input;
        if((int)status.size() > screenCols) status = status.substr(0, screenCols);
        cout << status;
        for(int i = status.size(); i < screenCols; i++) cout << " ";
        cout << "\x1b[m\x1b[K"; // Reset formatting and clear to end
        
        // Position cursor at end of input on status bar
        int cursorCol = prompt.length() + input.length() + 1;
        cout << "\x1b[" << screenRows << ";" << cursorCol << "H";
        cout << "\x1b[?25h"; // Show cursor
        cout.flush();

        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if(n <= 0) continue;

        if(ch == '\r') return input; // Enter
        else if((unsigned char)ch == 27) return ""; // Escape
        else if(ch == 127 || ch == 8) { // Backspace
            if(!input.empty()) input.pop_back();
        } else if(isprint((unsigned char)ch)) {
            input += ch;
        }
    }
}

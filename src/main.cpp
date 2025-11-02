#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <cstdlib>
#include "editor.h"
#include "syntax.h"
using namespace std;

class RawMode {
    struct termios orig_;
    public:
        RawMode() {
            if(tcgetattr(STDIN_FILENO, &orig_) == -1) {
                perror("tcgetattr");
                exit(EXIT_FAILURE);
            }
            struct termios raw = orig_;
            raw.c_lflag &= ~(ECHO | ICANON | ISIG); // Disable echo, canonical mode, and signals (Ctrl-C, Ctrl-Z)
            raw.c_iflag &= ~(IXON); // Disable software flow control (Ctrl-S, Ctrl-Q)
            raw.c_iflag &= ~(ICRNL); // Disable CR to NL mapping
            raw.c_oflag &= ~(OPOST); // Disable all output processing

            if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
                perror("tcsetattr");
                exit(EXIT_FAILURE);
            }
        }
        ~RawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_); }
};

int main(int argc, char* argv[]) {
    if(argc >= 1 && argv[0]) Syntax::setExecutablePath(argv[0]);
    RawMode rawMode;
    Editor editor;

    if(argc >= 2) editor.openFile(argv[1]);

    while(true) {
        editor.refreshScreen();
        if(!editor.processKeypress()) break;
    }
    return 0;
}

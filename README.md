# tedit
A simple TUI - Terminal Text Editor written in C++.

## Features
- A terminal-based text editor.
- Supports custom syntax highlighting.
- Supports custom theming and color schemes.
- Easy navigation with arrow keys.
- Open and save files.
- Rename and save as files.
- Undo and Redo functionality.
- Tabs and Auto indentation.
- Helpful status bar with necessary information.

## Build Instructions
1. Ensure you have a C++ compiler that supports C++17 or later. Also, make sure you have CMake installed.
2. Clone the repository:
```bash
git clone https://github.com/AaryanKhClasses/tedit
cd tedit
```
3. Build the project using CMake:
```bash
mkdir build && cd build
cmake ..
make
```
4. Run the editor:
```bash
./tedit
```
5. Optionally, you can specify a file to open:
```bash
./tedit ./test/test.cpp
```

> [!NOTE]
> If you don't want to build from source, you can download pre-built binaries from the [Releases](https://github.com/AaryanKhClasses/tedit/releases) page.
>
> For Linux, you might need to give execute permissions using:
> ```bash
> chmod +x tedit
> ```
>
> For Windows, you need to execute the binary in WSL or use a compatible terminal emulator.

## Usage
- Use arrow keys to navigate through the text.
- Press `Ctrl+S` to save the file.
- Press `Ctrl+W` to save as a new file.
- Press `Ctrl+R` to rename the file.
- Press `Ctrl+Q` to quit the editor.
- Press `Ctrl+Z` to undo the last action.
- Press `Ctrl+Y` to redo the last undone action.

## Customization
You can customize the editor by modifying the `themes` and `languages` directories. Add your own themes and syntax highlighting rules as needed.
### Languages
```json
{
    "name": "",
    "extensions": [""],
    "keywords": [""],
    "singleLineComments": "",
}
```

### Themes
```json
{
    "name": "",
    "colors": {
        "keyword": "",
        "number": "",
        "string": "",
        "comment": "",
        "text": "",
        "background": ""
    }
}
```

> [!NOTE]
> As of right now, multiple themes isn't supported. If you want to change the colors, then modify the `themes/default.json` file directly. Use ANSI color codes for the colors.

## License
This project is licensed under the GNU General Public License v3.0. See the [LICENSE](./LICENSE) file for details.

This project is owned by [AaryanKh](https://github.com/AaryanKhClasses). All rights reserved.

Feel free to contribute and report issues!

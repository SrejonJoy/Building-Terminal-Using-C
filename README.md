# Building-Terminal-Using-C

![Terminal Visual](docs/terminal-visual.png)

## Overview

**Building-Terminal-Using-C** is a custom, lightweight terminal emulator built from scratch in C. Its aim is to provide a simplified command-line interface for executing basic shell commands, managing files, and demonstrating the power of C for systems programming.

## Features

- Execute built-in shell commands (e.g., `ls`, `cd`, `pwd`, `echo`)
- Support for custom commands
- Command history
- User-friendly prompt
- Error handling for invalid commands
- Modular codebase for easy extension

## Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/SrejonJoy/Building-Terminal-Using-C.git
   cd Building-Terminal-Using-C
   ```
2. **Compile the terminal:**
   ```bash
   gcc -o my_terminal main.c
   ```
   *(Replace `main.c` with your entry source if different)*

3. **Run the terminal:**
   ```bash
   ./my_terminal
   ```

## Usage

Type any supported command and press ENTER. For example:
```
$ ls
$ cd ..
$ pwd
$ echo "Hello World"
```
Use `exit` to quit the terminal.

## Project Structure

```
Building-Terminal-Using-C/
├── main.c
├── commands.c
├── commands.h
├── README.md
└── docs/
    └── terminal-visual.png
```

## Example Visual

Here's a simple ASCII art representation of your terminal:

```
 ____________________________
|   SrejonJoy's C Terminal   |
|----------------------------|
| $ [user@machine] >         |
|----------------------------|
| Type commands below        |
|____________________________|
```

*(Consider replacing this with a PNG, SVG, or more advanced diagram if you wish!)*

## Contributing

Pull requests are welcome! For major changes, please open an issue first to discuss what you would like to change.

## License

[MIT](LICENSE)

---

*Made with ❤️ in C by SrejonJoy*

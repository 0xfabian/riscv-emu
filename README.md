# RISC-V Emulator and Disassembler

**RISC-V Emulator and Disassembler** is an interactive environment for running and visualizing RISC-V programs. It allows you to load RISC-V code, see its memory layout, inspect registers, and step through instructions. A portion of memory is mapped to the screen, and you can view the graphical output in real-time. The emulator also includes a **Pong** game as a demonstration.

> **Note**: This emulator currently supports running simple RISC-V programs and allows memory and screen visualization. It is a work in progress.

## Features

- **RISC-V Emulator**: Compile and run RISC-V programs within the emulator environment.
- **Disassembler**: View the assembly code corresponding to the compiled RISC-V program.
- **Memory Visualization**: View the memory layout, with specific regions mapped to the screen and input keys.
- **Register Inspection**: See the current state of RISC-V registers during execution.
- **Step/Auto-Step Mode**: Step through the program manually or run automatically to observe execution.
- **Screen Mapping**: A region of memory is mapped to the screen, allowing you to see graphical output (e.g., for games like Pong).
  
## Example

Below is a GIF showing the **Pong** game running on the emulator, demonstrating the real-time memory and screen mapping:

![Pong Game](gifs/pong.gif)

The emulator not only runs the program but also maps part of memory to the screen, which allows for graphical output (like the Pong game) to be displayed.

## Dependencies

- **[SDL2](https://www.libsdl.org/)** - Manages windowing, input, and graphics rendering.
- **[SDL2_ttf](https://github.com/libsdl-org/SDL_ttf)** - Used for rendering text in the emulator interface.

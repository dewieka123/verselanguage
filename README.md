🛠️ VerseLanguage Compilation Guide

VerseLanguage (VL) is designed to be highly portable. Because it uses an OS Abstraction Layer, you can compile the exact same vl.c source code across macOS, Linux, and Windows using native C compilers.

Here are the prerequisites and compilation scripts for each operating system.

🍎 1. macOS (Apple Silicon & Intel)

Prerequisites

You need the Xcode Command Line Tools which includes the clang compiler and necessary libraries.
Open your terminal and run the following command to install them:

xcode-select --install


Compilation Command

This command compiles the source code into a Universal Binary (running natively on both M-series and Intel chips) with support for Readline and dynamic .dylib modules.

clang -O3 -arch x86_64 -arch arm64 -o vl vl.c -lreadline -lm -ldl


🐧 2. Linux (Ubuntu / Debian / etc.)

Prerequisites

You need GCC (GNU Compiler Collection) and the development headers for Readline.
Open your terminal and install the build essentials:

sudo apt update
sudo apt install build-essential libreadline-dev


Compilation Command

This command generates a highly optimized ELF executable with support for dynamic .so modules.

gcc -O3 -o vl vl.c -lreadline -lm -ldl


🪟 3. Windows

Prerequisites

You need MinGW (Minimalist GNU for Windows) to get the gcc compiler.

Download and install MSYS2.

Open the MSYS2 terminal and install the GCC toolchain by running:

pacman -S mingw-w64-ucrt-x86_64-gcc


Open your Windows Settings and add the MinGW bin folder (usually C:\msys64\ucrt64\bin) to your System Environment Variables (PATH).

Compilation Command

This command generates a native .exe executable. Note that Windows handles dynamic loading (.dll) via its native API (LoadLibrary), so we don't need the -ldl flag, and the fallback REPL doesn't require -lreadline.

gcc -O3 -o vl.exe vl.c -lm


🚀 Running the Interpreter

After a successful compilation, you can start using VerseLanguage!

Start the interactive REPL:

macOS/Linux: ./vl

Windows: .\vl.exe

Run a script file:

macOS/Linux: ./vl script.vl

Windows: .\vl.exe script.vl

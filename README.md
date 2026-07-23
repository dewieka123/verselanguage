# 🌌 VerseLanguage (VL)
**Code like a human. Execute like a machine.**

VerseLanguage is a dependency-free, cross-platform scripting language built from the ground up in pure C. Designed with one philosophy: **total control over the machine, with syntax that reads like your own thoughts.**

No bloated runtimes. No 50GB SDKs. No walled gardens. Just raw C speed executing directly on your hardware.

> **Version: 2.5-Multiverse**

---

## ✨ Why VerseLanguage?

- 🧠 **Human-First Syntax** — Uses `yes` and `no` instead of abstract booleans. Loops are explicit and declarative with `loop number` and `loop inf`.
- 🛡️ **Ironclad Mutability** — `var` for mutable variables. `stvar` for immutable constants. Simple, enforced, no surprises.
- 🔌 **Native Dynamic Modules** — The `with` keyword is a direct bridge to your OS dynamic linker. Load compiled C/C++ libraries (`.dll`, `.so`, `.dylib`) at runtime with zero overhead.
- 🧹 **Mark-and-Sweep Garbage Collector** — Memory is tracked and swept automatically across the entire Multiverse. No manual `free`, no leaks.
- 🌌 **Multiverse Execution** — Run up to 256 isolated execution environments (Verses). Switch between them mid-execution. Each Verse has its own memory, variables, and state.
- 👽 **Military-Grade Bytecode Obfuscation** — Compile `.vl` scripts into encrypted `.vlb` files using Junk Byte Injection and Dynamic Rolling XOR keys seeded from OS time.
- 👾 **Raw Bitwise & Memory Operations** — Treat strings as raw bytes. Extract binary, octal, and hexadecimal data directly from memory.
- ⚠️ **Unsafe Blocks** — Manually allocate raw memory, read and write byte-level data, and interact with simulated CPU registers — all inside guarded `unsafe {}` blocks.
- 🖥️ **CPU Awareness** — Detect CPU instruction sets (`avx2`, `sse4.2`) and simulate x64 register manipulation (`RAX`, `RBX`, `RCX`, `RDX`) at runtime.
- 🎨 **Terminal Color Output** — Native ANSI color support via the built-in `color()` function.
- 🎮 **First-Class Game Engine Support** — Pair with [gamengine](https://github.com/dewieka123/gamengine) for 2D/3D rendering, audio, video, and input — all scripted from `.vl` files.
- 🌍 **True Cross-Platform** — Windows 7/10/11, Linux (kernel 3/4/5/6), macOS, Android (Termux + NDK), iOS (jailbreak, Mach-O ARM64).

---

## 📦 Installation

### macOS / Darwin

**Prerequisites:** Homebrew and Readline. First, install the Homebrew package manager (if you haven't already), then install the `readline` library:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install readline
```

**Build:**

```bash
clang -O3 -o vl vl.c -lreadline -lm -ldl
```

and this works on both Intel and Apple Silicon Macs

---

### Linux / BSD

**Prerequisites:** GCC and readline. On Ubuntu/Debian:

```bash
sudo apt update && sudo apt install build-essential libreadline-dev
```

**Build:**

```bash
gcc -O3 -o vl vl.c -lreadline -lm -ldl
```

---

### Windows

**Prerequisites:** MinGW via MSYS2. Download MSYS2 from [msys2.org](https://www.msys2.org), open the MSYS2 UCRT64 terminal, and run:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc
```

Add the MinGW `bin` folder to your Windows `PATH`, then build:

```bash
gcc -O3 -o vl.exe vl.c -lm
```

> `-lreadline` and `-ldl` are not needed on Windows. VerseLanguage handles terminal input natively via `#ifdef _WIN32`.

---

## 💻 Syntax & Features

### 1. Variables & Constants

```
var score = 0;
stvar MAX_LIVES = 3;

var name = "VerseLanguage";
var running = yes;
var failed = no;
```

### 2. Functions

```
fc greet(name) {
    show("Hello, " + name);
}

greet("World");
```

### 3. Conditions

```
if (score > 100) {
    show("High score!");
} else {
    show("Keep going.");
}
```

### 4. Loops

```
loop number(10) {
    show("Processing...");
}

var counter = 0;
loop inf {
    counter = counter + 1;
    if (counter > 500) {
        return;
    }
}
```

### 5. Increment Operator

```
var x = 0;
x++;
show(x);
```

### 6. Arrays

```
var buffer = array();
array_push(buffer, 255);
array_push(buffer, 1024);
show(array_len(buffer));
show(array_get(buffer, 0));
array_remove_first(buffer);
```

### 7. Classes & Objects

```
class Player {
    fc init(name, hp) {
        this.name = name;
        this.hp = hp;
    }
    fc status() {
        show(this.name + " - HP: " + this.hp);
    }
}

var p = new Player("Hero", 100);
p.status();
p.hp++;
p.status();
```

### 8. Dynamic Module Injection

```
with "module\gamengine.dll";
with "module/gamengine.so";
with "module/gamengine.dylib";
```

### 9. Multiverse Execution

```
var v1 = verse_add();
var v2 = verse_add();

verse_switch(v1) {
    var data = "isolated in verse 1";
    show(data);
}

verse_switch(v2) {
    var data = "isolated in verse 2";
    show(data);
}

show(verse_list());
verse_delete(v1);
verse_delete(v2);
```

### 10. Unsafe Blocks — Raw Memory

Allocate raw memory and read/write individual bytes directly via pointer indexing. All operations strictly require an `unsafe {}` block.

```
unsafe {
    var buf = addMemAlloc(1024);

    buf[0] = 72;
    buf[1] = 101;
    buf[2] = 108;
    buf[3] = 108;
    buf[4] = 111;

    show(buf[0]);
    show(buf[1]);

    delMemAlloc(buf);
}
```

### 11. Unsafe Blocks — CPU Instruction Detection

Verify the host CPU supports a required instruction set before executing hardware-dependent code.

```
unsafe {
    require_cpu_instruction("avx2");
    show("AVX2 is supported.");
}
```

Supported flags: `"avx2"`, `"sse4.2"`. Triggers a runtime error if the CPU does not meet the requirement.

### 12. Unsafe Blocks — CPU Register Simulation

Simulate x64 register manipulation. Registers are stored in isolated heap memory — not actual hardware registers — providing an assembly-like API safely.

```
unsafe {
    set_cpu_register("RAX", 999);
    set_cpu_register("RBX", 42);

    var a = get_cpu_register("RAX");
    var b = get_cpu_register("RBX");
    show(a + b);
}
```

Available registers: `RAX`, `RBX`, `RCX`, `RDX`.

### 13. Raw Memory & Bitwise Operations

```
stvar payload = "Hello";
show(hexadecimal(payload));
show(binary(payload));
show(octal(payload));

show(hexadecimal_decode_ascii("48 65 6C 6C 6F"));
show(hexadecimal_decode("FF"));
show(binary_decode("01001000"));
show(binary_decode_ascii("01001000 01100101 01101100 01101100 01101111"));
show(octal_decode("110"));
show(octal_decode_ascii("110 145 154 154 157"));
```

### 14. Terminal Color Output

```
show(color("Success!", "green"));
show(color("Warning!", "yellow"));
show(color("Error!",   "red"));
show(color("Info",     "cyan"));
show(color("Note",     "blue"));
show(color("Default",  "white"));
show(color("Bold",     "bold"));
```

### 15. System Calls

```
NewFile("config.txt", "key=value");
show(ReadFile("config.txt"));
cmd("ping 8.8.8.8");
OpenFile("document.pdf");
show(getCurrentSystem());
```

### 16. String Operations

```
show(toLowerCase("Hello World"));
show(toHigherCase("hello world"));
show(str(42));
```

### 17. Type Casting

```
var n = int("42");
var f = float("3.14");
var d = double("3.14159");
var b = bool(1);
```

### 18. CLI Arguments

```
show(array_len(sys_args));
show(array_get(sys_args, 0));
```

```
vl main.vl arg1 arg2
```

---

## ⚙️ Running Scripts

**Run a script directly:**
```
vl main.vl
```

**Run with CLI arguments:**
```
vl main.vl arg1 arg2
```

**Compile to encrypted bytecode:**
```
vl compile main.vl -o secure_app.vlb
```

**Execute encrypted bytecode:**
```
vl -byte secure_app.vlb
```

---

## 🌌 Multiverse System

VerseLanguage 2.5 introduces the **Multiverse** — up to 256 fully isolated execution environments inside a single process. Each Verse has its own variables, memory, and state. The garbage collector sweeps all active Verses simultaneously.

| Function | Description |
|----------|-------------|
| `verse_add()` | Spawn a new Verse, returns its ID |
| `verse_delete(id)` | Destroy a Verse and free its memory |
| `verse_list()` | Print all active Verse IDs |
| `verse_switch(id) { }` | Execute a block inside the target Verse |

All scripts run in **Verse 0** by default. `verse_switch` temporarily moves execution into the target Verse and returns when the block finishes.

---

## ⚠️ Unsafe Zone

The `unsafe {}` block is VerseLanguage's opt-in gateway to low-level hardware interaction. Calling any unsafe function outside this block triggers an immediate runtime protection error.

| Function | Description |
|----------|-------------|
| `addMemAlloc(bytes)` | Allocate raw memory, returns a pointer |
| `delMemAlloc(ptr)` | Free a raw memory pointer |
| `ptr[offset]` | Read a byte from raw memory at offset |
| `ptr[offset] = val` | Write a byte to raw memory at offset |
| `require_cpu_instruction(flag)` | Assert CPU supports an instruction set |
| `set_cpu_register(name, value)` | Write a value to a simulated register |
| `get_cpu_register(name)` | Read a value from a simulated register |

---

## 🎮 Game Engine

VerseLanguage has a dedicated game engine module — **[gamengine](https://github.com/dewieka123/gamengine)** — built in C++ with Qt6, OpenGL, Assimp, and LibVLC. Load it with `with` and build full games directly from `.vl` files.

```
with "module\gamengine.dll";

gamengine.init_window("My Game", 800, 600);
gamengine.load_background("assets/bg.png", 0, 0, 800, 600);
gamengine.play_music("assets/bgm.mp3");

var px = 400;
var py = 300;
var prev_click = no;

loop inf {
    var mx = gamengine.get_mouse_x();
    var my = gamengine.get_mouse_y();
    var click_raw = gamengine.is_mouse_pressed("LEFT");
    var click = no;
    if (click_raw == yes && prev_click == no) { click = yes; }
    prev_click = click_raw;

    if (gamengine.is_key_pressed("LEFT") == yes)  { px = px - 5; }
    if (gamengine.is_key_pressed("RIGHT") == yes) { px = px + 5; }
    if (gamengine.is_key_pressed("UP") == yes)    { py = py - 5; }
    if (gamengine.is_key_pressed("DOWN") == yes)  { py = py + 5; }

    gamengine.add_wall(0, 0, 800, 10);
    gamengine.add_wall(0, 590, 800, 10);

    if (gamengine.check_collision(px, py, 32, 32) == no) {
        gamengine.draw_rect(px, py, 32, 32, 0.2, 0.6, 1.0);
    }

    gamengine.draw_text("Score: 0", 10, 30, 1.0, 1.0, 1.0);

    var open = gamengine.update();
    if (open == no) { return; }

    wait(0.016);
}
```

See the full API reference at [github.com/dewieka123/gamengine](https://github.com/dewieka123/gamengine).

---

## 🌍 Platform Support

| Platform | Status |
|----------|--------|
| Windows 7 / 10 / 11 (x64) | ✅ |
| Linux kernel 3 / 4 / 5 / 6 (x86_64) | ✅ |
| macOS (Intel + Apple Silicon universal) | ✅ |
| Android via Termux | ✅ |
| Android via NDK | ✅ |
| iOS (Jailbreak, Mach-O ARM64) | ✅ |

> **Why does the Linux ELF run on every kernel version?**
> VerseLanguage targets stable POSIX syscalls. Linux upholds a rule: *"We never break userspace."* A binary compiled today still runs on a kernel 3 machine from 2012. Windows does not make this guarantee, hence MinGW is required to build for Windows.

---

## 📦 Built-in Functions

### I/O

| Function | Description |
|----------|-------------|
| `show(...)` | Print one or more values to terminal |
| `mkinput(prompt)` | Read a line of input from the user |
| `color(text, color)` | Wrap text in ANSI terminal color |
| `clear` | Clear the terminal screen |

### Type Casting

| Function | Description |
|----------|-------------|
| `int(val)` | Cast to integer |
| `float(val)` | Cast to float |
| `double(val)` | Cast to double |
| `bool(val)` | Cast to boolean |
| `str(val)` | Cast to string |

### String

| Function | Description |
|----------|-------------|
| `toLowerCase(str)` | Convert string to lowercase |
| `toHigherCase(str)` | Convert string to uppercase |

### Arrays

| Function | Description |
|----------|-------------|
| `array()` | Create a new empty array |
| `array_push(arr, val)` | Append a value to the array |
| `array_get(arr, idx)` | Get value at index |
| `array_len(arr)` | Get array length |
| `array_remove_first(arr)` | Remove the first element |

### Bitwise & Memory

| Function | Description |
|----------|-------------|
| `binary(val)` | Convert to binary string |
| `hexadecimal(val)` | Convert to hex string |
| `octal(val)` | Convert to octal string |
| `binary_decode(str)` | Decode binary string to number |
| `hexadecimal_decode(str)` | Decode hex string to number |
| `octal_decode(str)` | Decode octal string to number |
| `binary_decode_ascii(str)` | Decode binary string to ASCII |
| `hexadecimal_decode_ascii(str)` | Decode hex string to ASCII |
| `octal_decode_ascii(str)` | Decode octal string to ASCII |

### Unsafe Zone

Only callable inside an `unsafe {}` block.

| Function | Description |
|----------|-------------|
| `addMemAlloc(bytes)` | Allocate raw memory, returns pointer |
| `delMemAlloc(ptr)` | Free a raw memory pointer |
| `ptr[offset]` | Read byte at offset from raw pointer |
| `ptr[offset] = val` | Write byte at offset to raw pointer |
| `require_cpu_instruction(flag)` | Assert CPU instruction set support |
| `set_cpu_register(name, value)` | Write to simulated CPU register |
| `get_cpu_register(name)` | Read from simulated CPU register |

### System

| Function | Description |
|----------|-------------|
| `cmd(command)` | Execute a shell command |
| `NewFile(path, data)` | Write a file to disk |
| `ReadFile(path)` | Read a file from disk |
| `OpenFile(path)` | Open a file with the OS default app |
| `getCurrentSystem()` | Returns current OS name |
| `clock()` | Returns current timestamp |
| `wait(seconds)` | Sleep for given duration |

### Multiverse

| Function | Description |
|----------|-------------|
| `verse_add()` | Spawn a new Verse, returns its ID |
| `verse_delete(id)` | Destroy a Verse |
| `verse_list()` | List all active Verses |
| `verse_switch(id) { }` | Execute block in target Verse |

### Constants

| Name | Value |
|------|-------|
| `PI` | `3.14159265359` |
| `VERSION` | `"2.5-Multiverse"` |
| `sys_args` | Array of CLI arguments passed to the script |

---

## 🏗️ Architecture

```
VerseLanguage 2.5-Multiverse
├── Lexer            — Tokenizes source code into tokens
├── Parser           — Builds Abstract Syntax Tree (AST)
├── Interpreter      — Walks and evaluates the AST
├── GC               — Mark-and-sweep garbage collector (sweeps all Verses)
├── Multiverse       — Up to 256 isolated execution environments
├── Unsafe Zone      — Raw memory, pointer indexing, CPU register simulation
├── Module Loader    — dlopen / LoadLibraryA dynamic library bridge
├── Bytecode         — Rolling XOR encrypted .vlb compiler + runner
└── REPL             — Interactive multiline terminal (Verse-aware prompt)
```

---

## 🔗 Related

- 🎮 **[gamengine](https://github.com/dewieka123/gamengine)** — 2D/3D game engine module for VerseLanguage

---

## 🚀 Slogan

> **"Run Everywhere — From Windows to Your Fridge."**

*VerseLanguage — version 2.5-Multiverse*

# 🌌 VerseLanguage (VL)
**Code like a human. Execute like a machine.**

VerseLanguage is a dependency-free, cross-platform scripting language built from the ground up in pure C. Designed with one philosophy: **total control over the machine, with syntax that reads like your own thoughts.**

No bloated runtimes. No 50GB SDKs. No walled gardens. Just raw C speed executing directly on your hardware — from Windows 11 down to Windows 7, from Linux kernel 3 all the way to kernel 6, macOS, Android via Termux, and yes, even a jailbroken iPhone.

---

## ✨ Why VerseLanguage?

- 🧠 **Human-First Syntax** — Uses `yes` and `no` instead of abstract booleans. Loops are explicit and declarative with `loop number` and `loop inf`.
- 🛡️ **Ironclad Mutability** — `var` for mutable variables. `stvar` for immutable constants. Simple, enforced, no surprises.
- 🔌 **Native Dynamic Modules** — The `with` keyword is a direct bridge to your OS dynamic linker. Load compiled C/C++ libraries (`.dll`, `.so`, `.dylib`) at runtime with zero overhead.
- 🧹 **Mark-and-Sweep Garbage Collector** — Memory is tracked and swept automatically. No manual `free`, no leaks, no ARC nonsense choking your CPU in the background.
- 👽 **Military-Grade Bytecode Obfuscation** — Compile `.vl` scripts into encrypted `.vlb` files. The compiler uses Junk Byte Injection and Dynamic Rolling XOR keys seeded from OS time to protect your logic from reverse engineering.
- 👾 **Raw Bitwise & Memory Operations** — Treat strings as raw bytes. Extract binary, octal, and hexadecimal data directly from memory. Decode hex payloads back into readable data.
- 🎮 **First-Class Game Engine Support** — VerseLanguage has its own game engine (`gamengine`) built with Qt6, OpenGL, and LibVLC. Render 2D, 3D models via Assimp, play audio and video in-game, handle keyboard and mouse input — all scripted directly from `.vl` files.
- 🌍 **True Cross-Platform** — One codebase, one binary format, runs everywhere: Windows 7/10/11, Linux (kernel 3/4/5/6), macOS, Android (Termux + NDK), iOS (jailbreak). The ELF binary on Linux runs across all kernel versions because VerseLanguage targets stable POSIX syscalls that Linux has never broken since the beginning.

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

### 5. Arrays

```
var buffer = array();
array_push(buffer, 255);
array_push(buffer, 1024);
show(array_len(buffer));
array_remove_first(buffer);
```

### 6. Dynamic Module Injection

Load any compiled C/C++ shared library at runtime:

```
with "module\gamengine.dll";
with "module/gamengine.so";
with "module/gamengine.dylib";
```

### 7. Raw Memory & Bitwise Operations

```
stvar payload = "Hello";
show(hexadecimal(payload));
show(binary(payload));
show(octal(payload));

var dump = "48 65 6C 6C 6F";
show(hexadecimal_decode_ascii(dump));
```

### 8. System Calls

```
NewFile("config.txt", "key=value");
show(ReadFile("config.txt"));
cmd("ping 8.8.8.8");
OpenFile("document.pdf");
```

### 9. String Operations

```
var input = "sElEcT * fRoM";
show(toLowerCase(input));
show(toHigherCase(input));
show(strLen(input));
show(strReverse(input));
```


## ⚙️ Compilation & Execution

**Run a script directly:**
```
vl main.vl
```

**Compile to encrypted bytecode:**
```
vl compile main.vl -o secure_app.vlb
```

**Execute encrypted bytecode:**
```
vl -byte secure_app.vlb
```

**Compile VerseLanguage itself (Linux/Mac):**
```
clang vl.c -o vl -O3 -lm
```

**Compile on Windows:**
```
clang vl.c -o vl.exe -O3 -lm -D_WIN32_WINNT=0x0601
```

> `-lreadline` is NOT needed on Windows. VerseLanguage handles input natively per platform via `#ifdef _WIN32`.

---

## 🌍 Platform Support

| Platform | Status |
|----------|--------|
| Windows 7 / 10 / 11 (x64) | ✅ |
| Linux kernel 3 / 4 / 5 / 6 (x86_64) | ✅ |
| macOS | ✅ |
| Android via Termux | ✅ |
| Android via NDK | ✅ |
| iOS (Jailbreak, Mach-O ARM64) | ✅ |

> **Why does the Linux ELF run on all kernel versions?**
> Because VerseLanguage targets stable POSIX syscalls. Linux has a sacred rule: *"We never break userspace."* A binary compiled today runs on kernel 3 from 2012. Windows does not make the same guarantee — hence the platform flags needed for Windows 7 targeting.

---

## 📦 Built-in Functions

| Function | Description |
|----------|-------------|
| `show(...)` | Print to terminal |
| `mkinput(prompt)` | Read input from user |
| `binary(val)` | Convert to binary string |
| `hexadecimal(val)` | Convert to hex string |
| `octal(val)` | Convert to octal string |
| `hexadecimal_decode_ascii(str)` | Decode hex to ASCII |
| `binary_decode(str)` | Decode binary to value |
| `octal_decode(str)` | Decode octal to value |
| `toLowerCase(str)` | Lowercase string |
| `toHigherCase(str)` | Uppercase string |
| `strLen(str)` | String length |
| `strReverse(str)` | Reverse string |
| `int(num)` | Cast to integer |
| `clock()` | Current timestamp |
| `wait(seconds)` | Sleep |
| `cmd(command)` | Run OS shell command |
| `NewFile(path, data)` | Write file to disk |
| `ReadFile(path)` | Read file from disk |
| `OpenFile(path)` | Open file with OS default app |
| `getCurrentSystem()` | Detect current OS |
| `array()` | Create array |
| `array_push(arr, val)` | Push to array |
| `array_get(arr, idx)` | Get item by index |
| `array_len(arr)` | Array length |
| `array_remove_first(arr)` | Remove first element |

---

## 🏗️ Architecture

```
VerseLanguage
├── Lexer         — Tokenizes source code
├── Parser        — Builds Abstract Syntax Tree (AST)
├── Interpreter   — Walks and evaluates the AST
├── GC            — Mark-and-sweep garbage collector
├── Module Loader — dlopen / LoadLibraryA bridge
├── Bytecode      — Rolling XOR encrypted .vlb compiler
└── REPL          — Interactive multiline terminal
```

---

## 🚀 Slogan

> **"Run Everywhere — From Windows to Your Fridge."**

*VerseLanguage — version 2.5*

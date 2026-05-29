# 🌌 VerseLanguage (VL)
**Code like a human. Execute like a machine.**

VerseLanguage is a dependency-free, cross-platform scripting language built from the ground up in pure C. Designed with one absolute philosophy: **total control over the machine, with syntax that reads like your own thoughts.**

No bloated runtimes. No 50GB SDKs. No walled gardens. Just raw C speed executing directly on your hardware — from Windows 11 down to Windows 7, from Linux kernel 3 all the way to kernel 6, macOS, Android via Termux, and even a jailbroken iPhone.

> **Version: 2.5-Multiverse**

---

## ✨ Why VerseLanguage?

- 🧠 **Human-First Syntax** — Uses `yes` and `no` instead of abstract booleans. Loops are explicit and declarative with `loop number` and `loop inf`.
- 🛡️ **Ironclad Mutability** — `var` for mutable variables. `stvar` for immutable constants. Simple, enforced, no surprises.
- 🔌 **Native Dynamic Modules** — The `with` keyword is a direct bridge to your OS dynamic linker. Load compiled C/C++ libraries (`.dll`, `.so`, `.dylib`) at runtime with zero overhead.
- 🧹 **Mark-and-Sweep Garbage Collector** — Memory is tracked and swept automatically across the entire Multiverse. No manual `free`, no leaks, no ARC nonsense choking your CPU in the background.
- 🌌 **Multiverse Execution** — Run up to 256 isolated execution environments (Verses) in parallel. Switch between them mid-execution. Each Verse has its own memory, variables, and state.
- 👽 **Military-Grade Bytecode Obfuscation** — Compile `.vl` scripts into encrypted `.vlb` files using Junk Byte Injection and Dynamic Rolling XOR keys seeded from OS time.
- 👾 **Raw Bitwise & Memory Operations** — Treat strings as raw bytes. Extract binary, octal, and hexadecimal data directly from memory. Decode payloads back into values or ASCII.
- 🎨 **Terminal Color Output** — Native ANSI color support via the built-in `color()` function.
- 🌍 **True Cross-Platform** — One codebase runs everywhere: Windows 7/10/11, Linux (kernel 3/4/5/6), macOS, Android (Termux + NDK), iOS (jailbreak, Mach-O ARM64).

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
show(array_get(buffer, 0));
```

### 6. Classes & Objects

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
```

### 7. Dynamic Module Injection

```
with "module\gamengine.dll";
with "module/gamengine.so";
with "module/gamengine.dylib";
```

### 8. Multiverse Execution

Run up to 256 isolated execution environments. Each Verse has its own isolated memory.

```
var id = verse_add();

verse_switch(id) {
    show("Running inside Verse " + id);
    var x = 999;
}

show(verse_list());
verse_delete(id);
```

### 9. Raw Memory & Bitwise Operations

```
stvar payload = "Hello";
show(hexadecimal(payload));
show(binary(payload));
show(octal(payload));

var hex_dump = "48 65 6C 6C 6F";
show(hexadecimal_decode_ascii(hex_dump));
show(hexadecimal_decode(hex_dump));

var bin_str = "01001000";
show(binary_decode(bin_str));
show(binary_decode_ascii(bin_str));

show(octal_decode("110"));
show(octal_decode_ascii("110 145 154 154 157"));
```

### 10. Terminal Color Output

```
show(color("Success!", "green"));
show(color("Warning!", "yellow"));
show(color("Error!", "red"));
show(color("Info", "cyan"));
show(color("Note", "blue"));
show(color("Default", "white"));
show(color("Important", "bold"));
```

### 11. System Calls

```
NewFile("config.txt", "key=value");
show(ReadFile("config.txt"));
cmd("ping 8.8.8.8");
OpenFile("document.pdf");
show(getCurrentSystem());
```

### 12. String Operations

```
var input = "Hello World";
show(toLowerCase(input));
show(toHigherCase(input));
```

### 13. Type Casting

```
var n = int("42");
var f = float("3.14");
var d = double("3.14159");
var b = bool(1);
```

### 14. CLI Arguments

Arguments passed when running a script are automatically available as `sys_args`:

```
show(array_len(sys_args));
show(array_get(sys_args, 0));
```

Run with: `vl main.vl arg1 arg2`

---

## 🌌 Multiverse System

VerseLanguage 2.5 introduces the **Multiverse** — a system for running multiple fully isolated execution environments inside a single process.

| Function | Description |
|----------|-------------|
| `verse_add()` | Spawn a new isolated Verse, returns its ID |
| `verse_delete(id)` | Destroy a Verse and free its memory |
| `verse_list()` | Print all active Verses |
| `verse_switch(id) { }` | Execute a block inside the target Verse |

Maximum of **256 active Verses** at a time. The GC sweeps all active Verses simultaneously.

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

verse_delete(v1);
verse_delete(v2);
```

---

## ⚙️ Compilation & Execution

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

**Compile VerseLanguage itself — Linux / macOS:**
```
clang vl.c -o vl -O3 -lm
```

**Compile VerseLanguage itself — Windows:**
```
clang vl.c -o vl.exe -O3 -lm -D_WIN32_WINNT=0x0601
```

> `-lreadline` is **NOT** needed on Windows. VerseLanguage handles terminal input natively per platform via `#ifdef _WIN32`. The `0x0601` flag targets Windows 7 minimum.

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

> **Why does the Linux ELF run on every kernel version?**
> VerseLanguage targets stable POSIX syscalls. Linux upholds a sacred rule: *"We never break userspace."* A binary compiled today still runs on a kernel 3 machine from 2012. Windows does not make this guarantee — hence the `-D_WIN32_WINNT` flag for Windows 7 targeting.

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
| `binary(val)` | Convert number or string to binary |
| `hexadecimal(val)` | Convert number or string to hex |
| `octal(val)` | Convert number or string to octal |
| `binary_decode(str)` | Decode binary string to number |
| `hexadecimal_decode(str)` | Decode hex string to number |
| `octal_decode(str)` | Decode octal string to number |
| `binary_decode_ascii(str)` | Decode binary string to ASCII text |
| `hexadecimal_decode_ascii(str)` | Decode hex string to ASCII text |
| `octal_decode_ascii(str)` | Decode octal string to ASCII text |

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
├── GC               — Mark-and-sweep garbage collector (sweeps all active Verses)
├── Multiverse       — Up to 256 isolated execution environments
├── Module Loader    — dlopen / LoadLibraryA dynamic library bridge
├── Bytecode         — Rolling XOR encrypted .vlb compiler + runner
└── REPL             — Interactive multiline terminal (Verse-aware prompt)
```

---

## 🚀 Slogan

> **"Run Everywhere — From Windows to Your Fridge."**

*VerseLanguage — version 2.5-Multiverse*

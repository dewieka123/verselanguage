# 🌌 VerseLanguage (VL)
**Code like a human. Execute like a machine.**

VerseLanguage is a dependency-free, cross-platform scripting language built from the ground up in pure C. It was architected with a single philosophy: **programming should not look like math equations.** By replacing cryptic symbols with natural vocabulary and enforcing memory safety by design, VerseLanguage bridges the gap between absolute beginners and low-level system integrations.

---

## ✨ Why VerseLanguage?

* 🧠 **Human-First Logic:** Say goodbye to `for(int i=0; i<n; i++)`. In VL, you simply write `loop angka (10)`. Logic uses `yes` and `no` instead of abstract booleans.
* 🛡️ **Ironclad Mutability:** Protect your data instinctively. Use `var` for variables that change, and `stvar` (Static Variable) for constants. No more guessing.
* 🔌 **Native Dynamic Modules:** Extend the language infinitely. The `with` keyword acts as a direct bridge to your OS's dynamic linker, allowing you to load compiled C/Objective-C libraries (`.dylib`, `.so`, `.dll`) on the fly.
* ⚡ **Zero Bloat:** No massive runtimes or virtual machines. Just one lightweight, lightning-fast C executable compiled natively for your architecture.

---

## 💻 A Quick Look

VerseLanguage is highly readable. Here is an example of loading a native floating-point module and interacting with the user:

```verse
// Load native OS module dynamically
with "./float.dylib";

// Define a static (immutable) variable
stvar greeting = "Welcome to VerseLanguage Calculator!";
show(greeting);

// Get user input and convert it
var user_input = mkinput("Enter a decimal number: ");
var number = float.parse(user_input);

// Simple, expressive logic
if (number > 100) {
    show("That's a massive number!");
} else {
    show("Result multiplied by 2:", number * 2);
}

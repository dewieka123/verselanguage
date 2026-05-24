#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
char* strndup(const char* s, size_t n) {
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    strncpy(p, s, n);
    p[n] = '\0';
    return p;
}
#endif
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <setjmp.h>

// ========================================================================
// OS ABSTRACTION LAYER (Cross-Platform Magic: Windows vs macOS/Linux)
// ========================================================================
#ifdef _WIN32
    #include <windows.h>
    
    void* os_dlopen(const char* path) { return LoadLibraryA(path); }
    void* os_dlsym(void* handle, const char* symbol) { return (void*)GetProcAddress((HMODULE)handle, symbol); }
    const char* os_dlerror() {
        static char err_msg[512];
        DWORD err_code = GetLastError();
        if (err_code == 0) return "No error.";
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       err_msg, sizeof(err_msg), NULL);
        return err_msg;
    }
    void os_sleep(double seconds) { Sleep((DWORD)(seconds * 1000)); }
    double os_get_time() { return (double)GetTickCount64() / 1000.0; }
    char* os_readline(const char* prompt) {
        printf("%s", prompt); char buffer[1024];
        if (fgets(buffer, sizeof(buffer), stdin)) { buffer[strcspn(buffer, "\n")] = 0; return strdup(buffer); }
        return NULL;
    }
    void os_add_history(const char* line) { }

    // --- WIN32 KERNEL FILE SYSTEM CALLS ---
    bool os_write_file_sys(const char* path, const char* content) {
        HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;
        DWORD bytesWritten; WriteFile(hFile, content, strlen(content), &bytesWritten, NULL);
        CloseHandle(hFile); return true;
    }
    char* os_read_file_sys(const char* path) {
        HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return NULL;
        DWORD fileSize = GetFileSize(hFile, NULL);
        char* buffer = malloc(fileSize + 1); DWORD bytesRead;
        ReadFile(hFile, buffer, fileSize, &bytesRead, NULL);
        buffer[bytesRead] = '\0'; CloseHandle(hFile); return buffer;
    }
    void os_open_sys(const char* path) {
        char cmd[1024]; snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", path);
        system(cmd); // Eksekusi langsung ke Windows Shell
    }

#else
    // POSIX (Linux & UNIX)
    #include <sys/time.h>
    #include <unistd.h>
    #include <dlfcn.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <readline/readline.h>
    #include <readline/history.h>

    void* os_dlopen(const char* path) { return dlopen(path, RTLD_NOW | RTLD_GLOBAL); }
    void* os_dlsym(void* handle, const char* symbol) { return dlsym(handle, symbol); }
    const char* os_dlerror() { return dlerror(); }
    void os_sleep(double seconds) { usleep((useconds_t)(seconds * 1000000)); }
    double os_get_time() {
        struct timeval tv; gettimeofday(&tv, NULL);
        return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
    }
    char* os_readline(const char* prompt) { return readline(prompt); }
    void os_add_history(const char* line) { add_history(line); }

    // --- POSIX KERNEL FILE SYSTEM CALLS ---
    bool os_write_file_sys(const char* path, const char* content) {
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return false;
        write(fd, content, strlen(content)); close(fd); return true;
    }
    char* os_read_file_sys(const char* path) {
        int fd = open(path, O_RDONLY);
        if (fd < 0) return NULL;
        struct stat st; fstat(fd, &st);
        char* buffer = malloc(st.st_size + 1);
        read(fd, buffer, st.st_size); buffer[st.st_size] = '\0';
        close(fd); return buffer;
    }
    void os_open_sys(const char* path) {
        char cmd[1024];
        #ifdef __APPLE__
            snprintf(cmd, sizeof(cmd), "open \"%s\"", path); // <-- Mac manja
        #else
            snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", path); // <-- Linux brutal
        #endif
        system(cmd);
    }
#endif

// ==================== Error Handling & Core Structures ====================
jmp_buf error_jump;
bool has_error = false;

// Tambahkan VAL_ARRAY di akhir enum
typedef enum { VAL_NIL, VAL_NUM, VAL_BOOL, VAL_STR, VAL_NATIVE, VAL_FUNC, VAL_MODULE, VAL_ARRAY } ValType;

struct Value;
typedef struct Value (*NativeFn)(int arg_count, struct Value* args);

typedef struct Value {
    ValType type;
    union { 
        double num; bool boolean; char* str; 
        NativeFn native; struct FuncObj* func; void* module_handle; 
        void* array_data; // Tambahkan pointer untuk menyimpan struktur array
    } as;
} Value;

Value return_value;
bool is_returning = false;

void runtime_error(const char* message) {
    printf("Runtime Error: %s\n", message);
    has_error = true;
    longjmp(error_jump, 1);
}


void parse_error(int line, const char* message) {
    printf("Parse Error at line %d: %s\n", line, message);
    has_error = true;
    longjmp(error_jump, 1);
}

// ==================== Memory & Values ====================
Value make_num(double n) { Value v; v.type = VAL_NUM; v.as.num = n; return v; }
Value make_bool(bool b) { Value v; v.type = VAL_BOOL; v.as.boolean = b; return v; }
Value make_str(const char* s) { Value v; v.type = VAL_STR; v.as.str = strdup(s); return v; }
Value make_nil() { Value v; v.type = VAL_NIL; return v; }
Value make_native(NativeFn fn) { Value v; v.type = VAL_NATIVE; v.as.native = fn; return v; }
Value make_module(void* handle) { Value v; v.type = VAL_MODULE; v.as.module_handle = handle; return v; }

bool is_truthy(Value v) {
    if (v.type == VAL_NIL) return false;
    if (v.type == VAL_BOOL) return v.as.boolean;
    if (v.type == VAL_NUM) return v.as.num != 0;
    if (v.type == VAL_STR) return strlen(v.as.str) > 0;
    return true;
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    if (a.type == VAL_NIL) return true;
    if (a.type == VAL_NUM) return a.as.num == b.as.num;
    if (a.type == VAL_BOOL) return a.as.boolean == b.as.boolean;
    if (a.type == VAL_STR) return strcmp(a.as.str, b.as.str) == 0;
    return false;
}

void print_value(Value v) {
    if (v.type == VAL_NUM) printf("%g", v.as.num);
    else if (v.type == VAL_BOOL) printf("%s", v.as.boolean ? "yes" : "no");
    else if (v.type == VAL_STR) printf("%s", v.as.str);
    else if (v.type == VAL_NATIVE) printf("<native fn>");
    else if (v.type == VAL_FUNC) printf("<fc>");
    else if (v.type == VAL_MODULE) printf("<module>");
    else if (v.type == VAL_ARRAY) printf("<array>"); // <-- BARIS ARRAY MASUK DI SINI
    else printf("nil");
}

// ==================== AST Forward Declarations ====================
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Environment Environment;

typedef struct FuncObj {
    char* name; char** params; int param_count;
    Stmt* body; Environment* closure;
} FuncObj;

// ==================== Lexer (Tokenizer) ====================
typedef enum {
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_SEMI, TOKEN_COMMA, TOKEN_DOT,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MOD,
    TOKEN_ASSIGN, TOKEN_EQ, TOKEN_NEQ, TOKEN_LT, TOKEN_LTE, TOKEN_GT, TOKEN_GTE,
    TOKEN_NOT, TOKEN_AND, TOKEN_OR,
    TOKEN_IDENT, TOKEN_NUMBER, TOKEN_STRING,
    TOKEN_FC, TOKEN_LOOP, TOKEN_NUMBER_KW, TOKEN_INF,
    TOKEN_IF, TOKEN_ELSE, TOKEN_VAR, TOKEN_STVAR, TOKEN_RETURN, TOKEN_WITH,
    TOKEN_YES, TOKEN_NO, TOKEN_EOF
} VL_TokenType;

typedef struct { VL_TokenType type; char* lexeme; int line; } Token;

Token* tokens = NULL;
int token_count = 0;
int token_capacity = 0;

void add_token(VL_TokenType type, const char* lexeme, int line) {
    if (token_count >= token_capacity) {
        token_capacity = token_capacity < 8 ? 8 : token_capacity * 2;
        tokens = realloc(tokens, sizeof(Token) * token_capacity);
    }
    tokens[token_count].type = type;
    tokens[token_count].lexeme = lexeme ? strdup(lexeme) : NULL;
    tokens[token_count].line = line;
    token_count++;
}

VL_TokenType check_keyword(const char* ident) {
    if (strcmp(ident, "fc") == 0) return TOKEN_FC;
    if (strcmp(ident, "loop") == 0) return TOKEN_LOOP;
    if (strcmp(ident, "number") == 0) return TOKEN_NUMBER_KW;
    if (strcmp(ident, "inf") == 0) return TOKEN_INF;
    if (strcmp(ident, "if") == 0) return TOKEN_IF;
    if (strcmp(ident, "else") == 0) return TOKEN_ELSE;
    if (strcmp(ident, "var") == 0) return TOKEN_VAR;
    if (strcmp(ident, "stvar") == 0) return TOKEN_STVAR;
    if (strcmp(ident, "return") == 0) return TOKEN_RETURN;
    if (strcmp(ident, "with") == 0) return TOKEN_WITH;
    if (strcmp(ident, "yes") == 0) return TOKEN_YES;
    if (strcmp(ident, "no") == 0) return TOKEN_NO;
    return TOKEN_IDENT;
}

void tokenize(const char* source) {
    token_count = 0; int line = 1; int i = 0; int len = strlen(source);
    while (i < len) {
        char c = source[i];
        if (isspace(c)) { if (c == '\n') line++; i++; continue; }
        if (c == '/' && i + 1 < len && source[i+1] == '/') { while (i < len && source[i] != '\n') i++; continue; }
        if (c == '(') { add_token(TOKEN_LPAREN, "(", line); i++; }
        else if (c == ')') { add_token(TOKEN_RPAREN, ")", line); i++; }
        else if (c == '{') { add_token(TOKEN_LBRACE, "{", line); i++; }
        else if (c == '}') { add_token(TOKEN_RBRACE, "}", line); i++; }
        else if (c == ';') { add_token(TOKEN_SEMI, ";", line); i++; }
        else if (c == ',') { add_token(TOKEN_COMMA, ",", line); i++; }
        else if (c == '.') { add_token(TOKEN_DOT, ".", line); i++; }
        else if (c == '+') { add_token(TOKEN_PLUS, "+", line); i++; }
        else if (c == '-') { add_token(TOKEN_MINUS, "-", line); i++; }
        else if (c == '*') { add_token(TOKEN_STAR, "*", line); i++; }
        else if (c == '/') { add_token(TOKEN_SLASH, "/", line); i++; }
        else if (c == '%') { add_token(TOKEN_MOD, "%", line); i++; }
        else if (c == '!') {
            if (i+1 < len && source[i+1] == '=') { add_token(TOKEN_NEQ, "!=", line); i += 2; }
            else { add_token(TOKEN_NOT, "!", line); i++; }
        }
        else if (c == '=') {
            if (i+1 < len && source[i+1] == '=') { add_token(TOKEN_EQ, "==", line); i += 2; }
            else { add_token(TOKEN_ASSIGN, "=", line); i++; }
        }
        else if (c == '<') {
            if (i+1 < len && source[i+1] == '=') { add_token(TOKEN_LTE, "<=", line); i += 2; }
            else { add_token(TOKEN_LT, "<", line); i++; }
        }
        else if (c == '>') {
            if (i+1 < len && source[i+1] == '=') { add_token(TOKEN_GTE, ">=", line); i += 2; }
            else { add_token(TOKEN_GT, ">", line); i++; }
        }
        else if (c == '&' && i+1 < len && source[i+1] == '&') { add_token(TOKEN_AND, "&&", line); i += 2; }
        else if (c == '|' && i+1 < len && source[i+1] == '|') { add_token(TOKEN_OR, "||", line); i += 2; }
        else if (c == '"') {
            i++; int start = i;
            while (i < len && source[i] != '"') { if (source[i] == '\n') line++; i++; }
            if (i >= len) parse_error(line, "Unterminated string.");
            char* str = strndup(&source[start], i - start);
            add_token(TOKEN_STRING, str, line); free(str); i++;
        }
        else if (isdigit(c)) {
            int start = i;
            while (i < len && (isdigit(source[i]) || source[i] == '.')) i++;
            char* num_str = strndup(&source[start], i - start);
            add_token(TOKEN_NUMBER, num_str, line); free(num_str);
        }
        else if (isalpha(c) || c == '_') {
            int start = i;
            while (i < len && (isalnum(source[i]) || source[i] == '_')) i++;
            char* ident = strndup(&source[start], i - start);
            add_token(check_keyword(ident), ident, line); free(ident);
        }
        else { char err[50]; snprintf(err, sizeof(err), "Unexpected char: %c", c); parse_error(line, err); i++; }
    }
    add_token(TOKEN_EOF, "EOF", line);
}

// ==================== AST Structures & Parser ====================
typedef enum { EXPR_LITERAL, EXPR_VAR, EXPR_ASSIGN, EXPR_BINARY, EXPR_UNARY, EXPR_CALL, EXPR_GET } ExprType;
struct Expr {
    ExprType type;
    union {
        Value literal; char* name;
        struct { char* name; Expr* value; } assign;
        struct { Expr* left; Token op; Expr* right; } binary;
        struct { Token op; Expr* right; } unary;
        struct { Expr* callee; Expr** args; int arg_count; } call;
        struct { Expr* object; char* property; } get;
    } as;
};

typedef enum { STMT_VAR, STMT_BLOCK, STMT_IF, STMT_LOOP_NUMBER, STMT_LOOP_INF, STMT_FUNC, STMT_RETURN, STMT_WITH, STMT_EXPR } StmtType;
struct Stmt {
    StmtType type;
    union {
        Expr* expr;
        struct { char* name; Expr* init; bool is_mut; } var_decl;
        struct { Stmt** stmts; int count; } block;
        struct { Expr* cond; Stmt* then_b; Stmt* else_b; } if_stmt;
        struct { Expr* count; Stmt* body; } loop_number; // <-- UBAH INI
        Stmt* loop_inf_body;
        FuncObj* func_decl;
        Expr* with_path;
    } as;
};

Expr* new_expr(ExprType type) { Expr* e = malloc(sizeof(Expr)); e->type = type; return e; }
Stmt* new_stmt(StmtType type) { Stmt* s = malloc(sizeof(Stmt)); s->type = type; return s; }

int current_tok = 0;
Token peek() { return tokens[current_tok]; }
Token previous() { return tokens[current_tok - 1]; }
bool is_at_end() { return peek().type == TOKEN_EOF; }
Token advance() { if (!is_at_end()) current_tok++; return previous(); }
bool check(VL_TokenType type) { if (is_at_end()) return false; return peek().type == type; }
bool match(VL_TokenType type) { if (check(type)) { advance(); return true; } return false; }

Expr* parse_expr(); Stmt* parse_stmt();

Expr* parse_primary() {
    if (match(TOKEN_YES)) { Expr* e = new_expr(EXPR_LITERAL); e->as.literal = make_bool(true); return e; }
    if (match(TOKEN_NO)) { Expr* e = new_expr(EXPR_LITERAL); e->as.literal = make_bool(false); return e; }
    if (match(TOKEN_NUMBER)) { Expr* e = new_expr(EXPR_LITERAL); e->as.literal = make_num(atof(previous().lexeme)); return e; }
    if (match(TOKEN_STRING)) { Expr* e = new_expr(EXPR_LITERAL); e->as.literal = make_str(previous().lexeme); return e; }
    if (match(TOKEN_IDENT)) { Expr* e = new_expr(EXPR_VAR); e->as.name = strdup(previous().lexeme); return e; }
    if (match(TOKEN_LPAREN)) {
        Expr* expr = parse_expr();
        if (!match(TOKEN_RPAREN)) parse_error(peek().line, "Expected ')' after expression.");
        return expr;
    }
    parse_error(peek().line, "Expected expression."); return NULL;
}

Expr* parse_call() {
    Expr* expr = parse_primary();
    while (true) {
        if (match(TOKEN_LPAREN)) {
            int cap = 4, count = 0; Expr** args = malloc(sizeof(Expr*) * cap);
            if (!check(TOKEN_RPAREN)) {
                do {
                    if (count >= cap) { cap *= 2; args = realloc(args, sizeof(Expr*) * cap); }
                    args[count++] = parse_expr();
                } while (match(TOKEN_COMMA));
            }
            if (!match(TOKEN_RPAREN)) parse_error(peek().line, "Expected ')' after arguments.");
            Expr* call = new_expr(EXPR_CALL);
            call->as.call.callee = expr; call->as.call.args = args; call->as.call.arg_count = count; expr = call;
        } else if (match(TOKEN_DOT)) {
            if (!match(TOKEN_IDENT)) parse_error(peek().line, "Expected property name after '.'.");
            Expr* get = new_expr(EXPR_GET);
            get->as.get.object = expr; get->as.get.property = strdup(previous().lexeme); expr = get;
        } else { break; }
    }
    return expr;
}

Expr* parse_unary() {
    if (match(TOKEN_NOT) || match(TOKEN_MINUS)) {
        Token op = previous(); Expr* right = parse_unary();
        Expr* e = new_expr(EXPR_UNARY); e->as.unary.op = op; e->as.unary.right = right; return e;
    }
    return parse_call();
}

Expr* parse_binary(Expr* (*next)(), VL_TokenType* ops, int op_count) {
    Expr* expr = next();
    while (true) {
        bool matched = false;
        for (int i = 0; i < op_count; i++) { if (match(ops[i])) { matched = true; break; } }
        if (!matched) break;
        Token op = previous(); Expr* right = next();
        Expr* bin = new_expr(EXPR_BINARY);
        bin->as.binary.left = expr; bin->as.binary.op = op; bin->as.binary.right = right; expr = bin;
    }
    return expr;
}

VL_TokenType mul_ops[] = {TOKEN_STAR, TOKEN_SLASH, TOKEN_MOD};
Expr* parse_multiplication() { return parse_binary(parse_unary, mul_ops, 3); }
VL_TokenType add_ops[] = {TOKEN_PLUS, TOKEN_MINUS};
Expr* parse_addition() { return parse_binary(parse_multiplication, add_ops, 2); }
VL_TokenType comp_ops[] = {TOKEN_LT, TOKEN_LTE, TOKEN_GT, TOKEN_GTE};
Expr* parse_comparison() { return parse_binary(parse_addition, comp_ops, 4); }
VL_TokenType eq_ops[] = {TOKEN_EQ, TOKEN_NEQ};
Expr* parse_equality() { return parse_binary(parse_comparison, eq_ops, 2); }
VL_TokenType and_ops[] = {TOKEN_AND};
Expr* parse_and() { return parse_binary(parse_equality, and_ops, 1); }
VL_TokenType or_ops[] = {TOKEN_OR};
Expr* parse_or() { return parse_binary(parse_and, or_ops, 1); }

Expr* parse_assignment() {
    Expr* expr = parse_or();
    if (match(TOKEN_ASSIGN)) {
        Expr* value = parse_assignment();
        if (expr->type == EXPR_VAR) {
            Expr* assign = new_expr(EXPR_ASSIGN);
            assign->as.assign.name = expr->as.name; assign->as.assign.value = value;
            free(expr); return assign;
        }
        parse_error(peek().line, "Invalid assignment target.");
    }
    return expr;
}

Expr* parse_expr() { return parse_assignment(); }

Stmt* parse_block() {
    Stmt* b = new_stmt(STMT_BLOCK);
    int cap = 8, count = 0; Stmt** stmts = malloc(sizeof(Stmt*) * cap);
    while (!check(TOKEN_RBRACE) && !is_at_end()) {
        if (count >= cap) { cap *= 2; stmts = realloc(stmts, sizeof(Stmt*) * cap); }
        stmts[count++] = parse_stmt();
    }
    if (!match(TOKEN_RBRACE)) parse_error(peek().line, "Expected '}' after block.");
    b->as.block.stmts = stmts; b->as.block.count = count; return b;
}

Stmt* parse_stmt() {
    if (match(TOKEN_LBRACE)) return parse_block();
    
    if (match(TOKEN_WITH)) {
        Expr* path = parse_expr();
        if (!match(TOKEN_SEMI)) parse_error(peek().line, "Expected ';' after with statement.");
        Stmt* s = new_stmt(STMT_WITH); s->as.with_path = path; return s;
    }
    
    if (match(TOKEN_VAR) || match(TOKEN_STVAR)) {
        bool is_mut = previous().type == TOKEN_VAR;
        if (!match(TOKEN_IDENT)) parse_error(peek().line, "Expected variable name.");
        char* name = strdup(previous().lexeme); Expr* init = NULL;
        if (match(TOKEN_ASSIGN)) init = parse_expr();
        if (!match(TOKEN_SEMI)) parse_error(peek().line, "Expected ';' after var decl.");
        Stmt* s = new_stmt(STMT_VAR);
        s->as.var_decl.name = name; s->as.var_decl.init = init; s->as.var_decl.is_mut = is_mut; return s;
    }
    if (match(TOKEN_IF)) {
        if (!match(TOKEN_LPAREN)) parse_error(peek().line, "Expected '(' after if");
        Expr* cond = parse_expr();
        if (!match(TOKEN_RPAREN)) parse_error(peek().line, "Expected ')' after if condition");
        Stmt* then_b = parse_stmt(); Stmt* else_b = NULL;
        if (match(TOKEN_ELSE)) else_b = parse_stmt();
        Stmt* s = new_stmt(STMT_IF);
        s->as.if_stmt.cond = cond; s->as.if_stmt.then_b = then_b; s->as.if_stmt.else_b = else_b; return s;
    }
    if (match(TOKEN_LOOP)) {
        if (match(TOKEN_NUMBER_KW)) {
            if (!match(TOKEN_LPAREN)) parse_error(peek().line, "Expected '(' after 'number'.");
            Expr* count = parse_expr();
            if (!match(TOKEN_RPAREN)) parse_error(peek().line, "Expected ')' after loop count.");
            Stmt* body = parse_stmt();
            Stmt* s = new_stmt(STMT_LOOP_NUMBER); s->as.loop_number.count = count; s->as.loop_number.body = body; return s;
        } else if (match(TOKEN_INF)) {
            Stmt* body = parse_stmt(); Stmt* s = new_stmt(STMT_LOOP_INF); s->as.loop_inf_body = body; return s;
        }
        parse_error(peek().line, "Expected 'number' or 'inf' after loop.");
    }
    if (match(TOKEN_FC)) {
        if (!match(TOKEN_IDENT)) parse_error(peek().line, "Expected function name");
        char* name = strdup(previous().lexeme);
        if (!match(TOKEN_LPAREN)) parse_error(peek().line, "Expected '('");
        
        int cap = 4, param_count = 0; char** params = malloc(sizeof(char*) * cap);
        if (!check(TOKEN_RPAREN)) {
            do {
                if (param_count >= cap) { cap *= 2; params = realloc(params, sizeof(char*) * cap); }
                if (!match(TOKEN_IDENT)) parse_error(peek().line, "Expected parameter name");
                params[param_count++] = strdup(previous().lexeme);
            } while (match(TOKEN_COMMA));
        }
        if (!match(TOKEN_RPAREN)) parse_error(peek().line, "Expected ')' after parameters");
        Stmt* body = parse_stmt();
        if (body->type != STMT_BLOCK) parse_error(peek().line, "Function body must be a block");
        
        FuncObj* func = malloc(sizeof(FuncObj));
        func->name = name; func->params = params; func->param_count = param_count; func->body = body;
        Stmt* s = new_stmt(STMT_FUNC); s->as.func_decl = func; return s;
    }
    if (match(TOKEN_RETURN)) {
        Expr* val = NULL;
        if (!check(TOKEN_SEMI)) val = parse_expr();
        if (!match(TOKEN_SEMI)) parse_error(peek().line, "Expected ';' after return.");
        Stmt* s = new_stmt(STMT_RETURN); s->as.expr = val; return s;
    }
    
    Expr* expr = parse_expr();
    if (!match(TOKEN_SEMI)) parse_error(peek().line, "Expected ';' after expression.");
    Stmt* s = new_stmt(STMT_EXPR); s->as.expr = expr; return s;
}

// ==================== Environment ====================
typedef struct EnvNode { char* name; Value value; bool is_mut; struct EnvNode* next; } EnvNode;
struct Environment { EnvNode* head; struct Environment* enclosing; };

Environment* global_env; Environment* current_env;

Environment* new_env(Environment* enclosing) {
    Environment* e = malloc(sizeof(Environment)); e->head = NULL; e->enclosing = enclosing; return e;
}

void define_var(Environment* env, const char* name, Value value, bool is_mut) {
    EnvNode* node = malloc(sizeof(EnvNode));
    node->name = strdup(name); node->value = value; node->is_mut = is_mut;
    node->next = env->head; env->head = node;
}

void assign_var(Environment* env, const char* name, Value value) {
    Environment* curr = env;
    while (curr) {
        EnvNode* node = curr->head;
        while (node) {
            if (strcmp(node->name, name) == 0) {
                if (!node->is_mut) { char err[100]; snprintf(err, 100, "Cannot assign immutable %s", name); runtime_error(err); }
                node->value = value; return;
            }
            node = node->next;
        }
        curr = curr->enclosing;
    }
    char err[100]; snprintf(err, 100, "Undefined variable: %s", name); runtime_error(err);
}

Value get_var(Environment* env, const char* name) {
    Environment* curr = env;
    while (curr) {
        EnvNode* node = curr->head;
        while (node) { if (strcmp(node->name, name) == 0) return node->value; node = node->next; }
        curr = curr->enclosing;
    }
    char err[100]; snprintf(err, 100, "Undefined variable: %s", name); runtime_error(err); return make_nil();
}

// ==================== Dynamic Module Helper ====================
// Extracts module name correctly handling both '/' (Unix) and '\' (Windows)
char* extract_module_name(const char* path) {
    const char* base1 = strrchr(path, '/');
    const char* base2 = strrchr(path, '\\');
    const char* base = base1 > base2 ? base1 : base2; 
    base = base ? base + 1 : path;
    
    char* name = strdup(base);
    char* dot = strrchr(name, '.');
    if (dot) *dot = '\0';
    return name;
}

// ==================== GARBAGE COLLECTOR (Mark & Sweep) ====================
typedef struct GCObject {
    Value value;
    bool marked;
    struct GCObject* next;
} GCObject;

GCObject* gc_head = NULL;

Value track_obj(Value v) {
    GCObject* obj = malloc(sizeof(GCObject));
    obj->value = v; obj->marked = false;
    obj->next = gc_head; gc_head = obj;
    return v;
}

Value make_gc_str(const char* s) { 
    Value v; v.type = VAL_STR; v.as.str = strdup(s); 
    return track_obj(v); 
}

void mark_value(Value v) {
    if (v.type != VAL_STR && v.type != VAL_ARRAY) return;
    GCObject* curr = gc_head;
    while (curr) {
        if (!curr->marked) {
            if (v.type == VAL_STR && curr->value.type == VAL_STR && curr->value.as.str == v.as.str) {
                curr->marked = true; break;
            }
            if (v.type == VAL_ARRAY && curr->value.type == VAL_ARRAY && curr->value.as.array_data == v.as.array_data) {
                curr->marked = true; 
                // Mark rekursif untuk elemen array
                struct VLArray { Value* elements; int count; int capacity; } *arr = v.as.array_data;
                for (int i = 0; i < arr->count; i++) mark_value(arr->elements[i]);
                break;
            }
        }
        curr = curr->next;
    }
}

void mark_env(Environment* env) {
    while (env) {
        EnvNode* node = env->head;
        while (node) { mark_value(node->value); node = node->next; }
        env = env->enclosing;
    }
}

void gc_collect() {
    // 1. Mark Phase (Hapus tanda lama, pasang tanda baru)
    GCObject* curr = gc_head;
    while (curr) { curr->marked = false; curr = curr->next; }
    mark_env(global_env);
    mark_env(current_env);
    
    // 2. Sweep Phase (Bakar memori yang tidak ditandai)
    GCObject** prev = &gc_head;
    curr = gc_head;
    while (curr) {
        if (!curr->marked) {
            GCObject* unreached = curr;
            *prev = curr->next; curr = curr->next;
            
            if (unreached->value.type == VAL_STR) free(unreached->value.as.str);
            else if (unreached->value.type == VAL_ARRAY) {
                struct VLArray { Value* elements; int count; int capacity; } *arr = unreached->value.as.array_data;
                free(arr->elements); free(arr);
            }
            free(unreached);
        } else {
            prev = &curr->next; curr = curr->next;
        }
    }
}

// ==================== Evaluator ====================
Value eval(Expr* expr); void execute(Stmt* stmt);

Value eval(Expr* expr) {
    if (expr->type == EXPR_LITERAL) return expr->as.literal;
    if (expr->type == EXPR_VAR) return get_var(current_env, expr->as.name);
    
    if (expr->type == EXPR_GET) {
        Value obj = eval(expr->as.get.object);
        if (obj.type != VAL_MODULE) runtime_error("Only modules have properties/functions.");
        
        NativeFn fn = (NativeFn)os_dlsym(obj.as.module_handle, expr->as.get.property);
        if (!fn) {
            char err[256];
            snprintf(err, sizeof(err), "Function '%s' not found in module.", expr->as.get.property);
            runtime_error(err);
        }
        return make_native(fn);
    }
    
    if (expr->type == EXPR_ASSIGN) {
        Value val = eval(expr->as.assign.value); assign_var(current_env, expr->as.assign.name, val); return val;
    }
    if (expr->type == EXPR_UNARY) {
        Value right = eval(expr->as.unary.right);
        if (expr->as.unary.op.type == TOKEN_MINUS) {
            if (right.type != VAL_NUM) runtime_error("Operand must be a number."); return make_num(-right.as.num);
        }
        if (expr->as.unary.op.type == TOKEN_NOT) return make_bool(!is_truthy(right));
    }
    if (expr->type == EXPR_BINARY) {
        Value left = eval(expr->as.binary.left); Value right = eval(expr->as.binary.right);
        VL_TokenType op = expr->as.binary.op.type;
        
        if (op == TOKEN_PLUS) {
            if (left.type == VAL_NUM && right.type == VAL_NUM) return make_num(left.as.num + right.as.num);
            if (left.type == VAL_STR || right.type == VAL_STR) {
                char b1[256], b2[256];
                if (left.type == VAL_STR) strcpy(b1, left.as.str); else snprintf(b1, 256, "%g", left.as.num);
                if (right.type == VAL_STR) strcpy(b2, right.as.str); else snprintf(b2, 256, "%g", right.as.num);
                char* comb = malloc(strlen(b1) + strlen(b2) + 1); sprintf(comb, "%s%s", b1, b2); 
                Value res = make_gc_str(comb); // <-- KUNCI! Teks ini sekarang dilacak GC!
                free(comb);
                return res;
            }
            runtime_error("Operands must be numbers or strings for +");
        }
        if (op == TOKEN_MINUS) { if (left.type != VAL_NUM || right.type != VAL_NUM) runtime_error("Numbers expected."); return make_num(left.as.num - right.as.num); }
        if (op == TOKEN_STAR) { if (left.type != VAL_NUM || right.type != VAL_NUM) runtime_error("Numbers expected."); return make_num(left.as.num * right.as.num); }
        if (op == TOKEN_SLASH) { if (left.type != VAL_NUM || right.type != VAL_NUM) runtime_error("Numbers expected."); if (right.as.num == 0) runtime_error("Div by 0."); return make_num(left.as.num / right.as.num); }
        if (op == TOKEN_MOD) { if (left.type != VAL_NUM || right.type != VAL_NUM) runtime_error("Numbers expected."); return make_num(fmod(left.as.num, right.as.num)); }
        
        if (op == TOKEN_LT) { if (left.type != VAL_NUM || right.type != VAL_NUM) runtime_error("Numbers expected."); return make_bool(left.as.num < right.as.num); }
        if (op == TOKEN_LTE) { if (left.type != VAL_NUM || right.type != VAL_NUM) runtime_error("Numbers expected."); return make_bool(left.as.num <= right.as.num); }
        if (op == TOKEN_GT) { if (left.type != VAL_NUM || right.type != VAL_NUM) runtime_error("Numbers expected."); return make_bool(left.as.num > right.as.num); }
        if (op == TOKEN_GTE) { if (left.type != VAL_NUM || right.type != VAL_NUM) runtime_error("Numbers expected."); return make_bool(left.as.num >= right.as.num); }
        if (op == TOKEN_EQ) return make_bool(values_equal(left, right));
        if (op == TOKEN_NEQ) return make_bool(!values_equal(left, right));
        if (op == TOKEN_AND) return make_bool(is_truthy(left) && is_truthy(right));
        if (op == TOKEN_OR) return make_bool(is_truthy(left) || is_truthy(right));
    }
    if (expr->type == EXPR_CALL) {
        Value callee = eval(expr->as.call.callee);
        Value* args = malloc(sizeof(Value) * expr->as.call.arg_count);
        for (int i = 0; i < expr->as.call.arg_count; i++) args[i] = eval(expr->as.call.args[i]);
        
        if (callee.type == VAL_NATIVE) {
            Value result = callee.as.native(expr->as.call.arg_count, args);
            free(args); return result;
        } else if (callee.type == VAL_FUNC) {
            FuncObj* fn = callee.as.func;
            if (expr->as.call.arg_count != fn->param_count) runtime_error("Incorrect number of arguments.");
            
            Environment* prev = current_env;
            current_env = new_env(fn->closure);
            for (int i = 0; i < fn->param_count; i++) define_var(current_env, fn->params[i], args[i], true);
            
            execute(fn->body);
            Value result = is_returning ? return_value : make_nil();
            
            is_returning = false;
            current_env = prev; free(args); return result;
        }
        runtime_error("Can only call functions.");
    }
    return make_nil();
}

void execute(Stmt* stmt) {
    if (has_error || is_returning) return;

    if (stmt->type == STMT_WITH) {
        Value path_val = eval(stmt->as.with_path);
        if (path_val.type != VAL_STR) runtime_error("Module path must be a string.");

        char target_path[1024];
        snprintf(target_path, sizeof(target_path), "%s", path_val.as.str);

        // Eksekusi mutasi path eksklusif untuk keluarga POSIX/UNIX
        #ifndef _WIN32
            // Jika nama modul murni (tidak mengandung '/' atau '.'), otomatis injeksi absolut path
            if (strchr(path_val.as.str, '/') == NULL && strchr(path_val.as.str, '.') == NULL) {
                #ifdef __APPLE__
                    // Ekstensi dylib kaku untuk meladeni OS Apple yang tertutup
                    snprintf(target_path, sizeof(target_path), "/usr/local/lib/%s.dylib", path_val.as.str);
                #else
                    // Ekstensi suci ELF .so untuk Linux yang berdaulat
                    snprintf(target_path, sizeof(target_path), "/usr/local/lib/%s.so", path_val.as.str);
                #endif
            }
        #endif

        // Load dynamic library menggunakan target_path yang sudah dimutasi
        void* handle = os_dlopen(target_path);
        if (!handle) {
            char err[512];
            snprintf(err, sizeof(err), "Failed to load module: %s", os_dlerror());
            runtime_error(err);
        }

        // Variabel di environment VL tetap menggunakan nama aslinya (contoh: "gamengine")
        char* module_name = extract_module_name(path_val.as.str);
        define_var(current_env, module_name, make_module(handle), false); 
        free(module_name);
        return;
    }

    if (stmt->type == STMT_VAR) {
        Value val = stmt->as.var_decl.init ? eval(stmt->as.var_decl.init) : make_num(0);
        define_var(current_env, stmt->as.var_decl.name, val, stmt->as.var_decl.is_mut);
    } else if (stmt->type == STMT_BLOCK) {
        Environment* prev = current_env; current_env = new_env(current_env);
        for (int i = 0; i < stmt->as.block.count; i++) execute(stmt->as.block.stmts[i]);
        current_env = prev;
    } else if (stmt->type == STMT_IF) {
        if (is_truthy(eval(stmt->as.if_stmt.cond))) execute(stmt->as.if_stmt.then_b);
        else if (stmt->as.if_stmt.else_b) execute(stmt->as.if_stmt.else_b);
    } else if (stmt->type == STMT_LOOP_NUMBER) {
        Value count = eval(stmt->as.loop_number.count);
        if (count.type != VAL_NUM) runtime_error("Loop count must be a number.");
        for (int i = 0; i < count.as.num; i++) {
            if (has_error || is_returning) break;
            execute(stmt->as.loop_number.body);
            gc_collect(); 
        }
    } else if (stmt->type == STMT_LOOP_INF) {
        while (true) {
            if (has_error || is_returning) break;
            execute(stmt->as.loop_inf_body);
            gc_collect(); // SAPU BERSIH SISA STRING SKOR GAME ENGINE!
        }
    } else if (stmt->type == STMT_FUNC) {
        stmt->as.func_decl->closure = current_env;
        Value fn_val; fn_val.type = VAL_FUNC; fn_val.as.func = stmt->as.func_decl;
        define_var(current_env, stmt->as.func_decl->name, fn_val, false);
    } else if (stmt->type == STMT_RETURN) {
        return_value = stmt->as.expr ? eval(stmt->as.expr) : make_nil();
        is_returning = true;
    } else if (stmt->type == STMT_EXPR) {
        eval(stmt->as.expr);
    }
}

// ==================== Native Functions ====================
// ==================== Native Array Implementation ====================
typedef struct {
    Value* elements;
    int count;
    int capacity;
} VLArray;

Value make_array(void* arr) { Value v; v.type = VAL_ARRAY; v.as.array_data = arr; return v; }

// array() -> membuat array kosong
Value native_array(int argCount, Value* args) {
    VLArray* arr = (VLArray*)malloc(sizeof(VLArray));
    arr->capacity = 4; // Kapasitas awal
    arr->count = 0;
    arr->elements = (Value*)malloc(sizeof(Value) * arr->capacity);
    return track_obj(make_array(arr)); // <-- KUNCI! Array dilacak GC!
}

// array_push(arr, nilai)
Value native_array_push(int argCount, Value* args) {
    if (argCount < 2 || args[0].type != VAL_ARRAY) return make_nil();
    VLArray* arr = (VLArray*)args[0].as.array_data;
    
    // Jika penuh, gandakan kapasitas memori
    if (arr->count >= arr->capacity) {
        arr->capacity *= 2;
        arr->elements = (Value*)realloc(arr->elements, sizeof(Value) * arr->capacity);
    }
    arr->elements[arr->count++] = args[1];
    return make_nil();
}

// array_get(arr, index)
Value native_array_get(int argCount, Value* args) {
    if (argCount < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_NUM) return make_nil();
    VLArray* arr = (VLArray*)args[0].as.array_data;
    int index = (int)args[1].as.num;
    
    if (index >= 0 && index < arr->count) return arr->elements[index];
    
    runtime_error("Index array di luar batas!");
    return make_nil();
}

// array_len(arr)
Value native_array_len(int argCount, Value* args) {
    if (argCount < 1 || args[0].type != VAL_ARRAY) return make_num(0);
    VLArray* arr = (VLArray*)args[0].as.array_data;
    return make_num((double)arr->count);
}

// array_remove_first(arr)
Value native_array_remove_first(int argCount, Value* args) {
    if (argCount < 1 || args[0].type != VAL_ARRAY) return make_nil();
    VLArray* arr = (VLArray*)args[0].as.array_data;
    
    if (arr->count > 0) {
        // Geser semua elemen ke kiri (menimpa index 0)
        for (int i = 1; i < arr->count; i++) {
            arr->elements[i - 1] = arr->elements[i];
        }
        arr->count--;
    }
    return make_nil();
}

Value native_clock(int argCount, Value* args) { return make_num(os_get_time()); }
Value native_wait(int argCount, Value* args) {
    if (argCount == 1 && args[0].type == VAL_NUM) os_sleep(args[0].as.num);
    return make_nil();
}
Value native_show(int argCount, Value* args) {
    for (int i = 0; i < argCount; i++) {
        if (args[i].type == VAL_STR && strcmp(args[i].as.str, "__INTERNAL_CLEAR__") == 0) {
            printf("\e[1;1H\e[2J"); continue;
        }
        print_value(args[i]);
        if (i < argCount - 1) printf(" "); 
    }
    printf("\n"); return make_nil();
}
Value native_mkinput(int argCount, Value* args) {
    if (argCount > 0) print_value(args[0]);
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) { buffer[strcspn(buffer, "\n")] = 0; return make_str(buffer); }
    return make_str("");
}

// Fungsi untuk mendapatkan nama OS saat ini
// Fungsi untuk mendapatkan nama OS saat ini (Sekarang makin lengkap!)
Value native_getCurrentSystem(int argCount, Value* args) {
    #ifdef _WIN32
        return make_str("Windows");
    #elif __APPLE__
        return make_str("macOS");
    #elif __linux__
        return make_str("Linux");
    #elif __FreeBSD__ || __OpenBSD__ || __NetBSD__
        return make_str("BSD");
    #elif __HAIKU__
        return make_str("HaikuOS");
    #else
        return make_str("Unknown");
    #endif
}

// Fungsi int(): Parse string ke integer, atau potong desimal dari angka
Value native_int(int argCount, Value* args) {
    if (argCount != 1) runtime_error("int() requires exactly 1 argument.");
    
    if (args[0].type == VAL_NUM) {
        return make_num((int)args[0].as.num); // Truncate desimal
    } else if (args[0].type == VAL_STR) {
        // Gunakan strtol bawaan C standar yang jauh lebih superior daripada 
        // mekanisme parsing Swift milik Apple yang sering crash kalau ada spasi tak terlihat.
        return make_num((int)strtol(args[0].as.str, NULL, 10));
    }
    return make_num(0); // Fallback kasar jika bukan string/angka
}

// Fungsi double() / float(): Parse string ke desimal berpresisi ganda
Value native_double(int argCount, Value* args) {
    if (argCount != 1) runtime_error("double() requires exactly 1 argument.");
    
    if (args[0].type == VAL_NUM) {
        return args[0]; // Sudah double
    } else if (args[0].type == VAL_STR) {
        return make_num(strtod(args[0].as.str, NULL));
    }
    return make_num(0);
}

// float() hanyalah alias redundan dari double() di arsitektur ini
Value native_float(int argCount, Value* args) {
    return native_double(argCount, args);
}

// Fungsi untuk menjalankan perintah terminal/CMD
Value native_cmd(int argCount, Value* args) {
    char command[4096] = ""; // Buffer untuk menggabungkan semua teks/variabel

    for (int i = 0; i < argCount; i++) {
        char temp[256];
        if (args[i].type == VAL_NUM) {
            snprintf(temp, sizeof(temp), "%g", args[i].as.num);
            strncat(command, temp, sizeof(command) - strlen(command) - 1);
        } else if (args[i].type == VAL_BOOL) {
            strncat(command, args[i].as.boolean ? "yes" : "no", sizeof(command) - strlen(command) - 1);
        } else if (args[i].type == VAL_STR) {
            strncat(command, args[i].as.str, sizeof(command) - strlen(command) - 1);
        }
        
        // Menambahkan spasi antar argumen seperti yang dilakukan fungsi show()
        if (i < argCount - 1) {
            strncat(command, " ", sizeof(command) - strlen(command) - 1);
        }
    }

    // Eksekusi string yang sudah digabungkan ke OS
    int status = system(command);
    
    // Mengembalikan status exit dari OS (0 = sukses)
    return make_num(status); 
}

// ==================== Bitwise & Base Converters ====================

// Fungsi hexadecimal(): Konversi angka desimal atau teks ASCII ke Hex
Value native_hexadecimal(int argCount, Value* args) {
    if (argCount != 1) runtime_error("hexadecimal() requires exactly 1 argument.");
    char buffer[8192] = ""; // Buffer besar untuk string panjang
    
    if (args[0].type == VAL_NUM) {
        snprintf(buffer, sizeof(buffer), "%X", (unsigned int)(long long)args[0].as.num);
    } else if (args[0].type == VAL_STR) {
        int len = strlen(args[0].as.str);
        int pos = 0;
        for (int i = 0; i < len && pos < 8000; i++) {
            pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%02X ", (unsigned char)args[0].as.str[i]);
        }
        if (pos > 0 && buffer[pos-1] == ' ') buffer[pos-1] = '\0'; // Hapus spasi sisa
    } else { return make_str(""); }
    
    return make_gc_str(buffer); // Masuk ke Garbage Collector!
}

// Fungsi octal(): Konversi angka desimal atau teks ASCII ke Oktal
Value native_octal(int argCount, Value* args) {
    if (argCount != 1) runtime_error("octal() requires exactly 1 argument.");
    char buffer[8192] = "";
    
    if (args[0].type == VAL_NUM) {
        snprintf(buffer, sizeof(buffer), "%o", (unsigned int)(long long)args[0].as.num);
    } else if (args[0].type == VAL_STR) {
        int len = strlen(args[0].as.str);
        int pos = 0;
        for (int i = 0; i < len && pos < 8000; i++) {
            pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%03o ", (unsigned char)args[0].as.str[i]);
        }
        if (pos > 0 && buffer[pos-1] == ' ') buffer[pos-1] = '\0';
    } else { return make_str(""); }
    
    return make_gc_str(buffer);
}

// Fungsi binary(): Konversi angka desimal atau teks ASCII ke Biner murni (0 dan 1)
Value native_binary(int argCount, Value* args) {
    if (argCount != 1) runtime_error("binary() requires exactly 1 argument.");
    char buffer[8192] = "";
    
    if (args[0].type == VAL_NUM) {
        unsigned int n = (unsigned int)(long long)args[0].as.num;
        if (n == 0) strcpy(buffer, "0");
        else {
            char temp[65]; int idx = 0;
            while (n > 0) { temp[idx++] = (n % 2) ? '1' : '0'; n /= 2; }
            for (int i = 0; i < idx; i++) buffer[i] = temp[idx - 1 - i];
            buffer[idx] = '\0';
        }
    } else if (args[0].type == VAL_STR) {
        int len = strlen(args[0].as.str);
        int pos = 0;
        for (int i = 0; i < len && pos < 8000; i++) {
            unsigned char c = args[0].as.str[i];
            for (int b = 7; b >= 0; b--) { // Ekstraksi 8-bit per karakter
                buffer[pos++] = ((c >> b) & 1) ? '1' : '0';
            }
            buffer[pos++] = ' '; // Spasi pemisah tiap huruf
        }
        if (pos > 0) buffer[pos-1] = '\0';
    } else { return make_str(""); }
    
    return make_gc_str(buffer);
}

// --- HELPER UNTUK DECODE KE TEKS ASCII ---
Value decode_to_ascii(Value* args, int base, const char* func_name) {
    if (args[0].type != VAL_STR) {
        char err[100]; snprintf(err, 100, "%s() requires a string argument.", func_name);
        runtime_error(err); return make_nil();
    }
    
    char* input = strdup(args[0].as.str); // Copy string agar aman dipotong strtok
    char* token = strtok(input, " ");
    int cap = 256; int len = 0;
    char* buffer = malloc(cap);
    
    while (token != NULL) {
        char* endptr;
        long val = strtol(token, &endptr, base);
        
        if (*endptr != '\0') {
            free(input); free(buffer);
            char err[150]; snprintf(err, 150, "Invalid characters detected in %s input.", func_name);
            runtime_error(err); return make_nil();
        }
        
        if (len + 1 >= cap) { cap *= 2; buffer = realloc(buffer, cap); }
        buffer[len++] = (char)val;
        token = strtok(NULL, " ");
    }
    buffer[len] = '\0';
    free(input);
    
    Value res = make_gc_str(buffer); // Masuk Garbage Collector
    free(buffer);
    return res;
}

// --- HELPER UNTUK DECODE KE ANGKA DESIMAL (NUMBER) ---
Value decode_to_number(Value* args, int base, const char* func_name) {
    if (args[0].type != VAL_STR) {
        char err[100]; snprintf(err, 100, "%s() requires a string argument.", func_name);
        runtime_error(err); return make_nil();
    }
    
    const char* str = args[0].as.str;
    while(isspace(*str)) str++; // Skip spasi di depan
    
    char* endptr;
    long long val = strtoll(str, &endptr, base);
    
    while(isspace(*endptr)) endptr++; // Skip spasi di belakang
    
    // Jika ada karakter sampah yang tidak valid sesuai basisnya
    if (*str == '\0' || *endptr != '\0') {
        char err[150]; snprintf(err, 150, "Invalid or malformed string for %s.", func_name);
        runtime_error(err); return make_nil();
    }
    
    return make_num((double)val);
}

// Eksekusi Wrapper Func: String -> Desimal Number
Value native_binary_decode(int argCount, Value* args) { return decode_to_number(args, 2, "binary_decode"); }
Value native_hexadecimal_decode(int argCount, Value* args) { return decode_to_number(args, 16, "hexadecimal_decode"); }
Value native_octal_decode(int argCount, Value* args) { return decode_to_number(args, 8, "octal_decode"); }

// Eksekusi Wrapper Func: String -> ASCII Text
Value native_binary_decode_ascii(int argCount, Value* args) { return decode_to_ascii(args, 2, "binary_decode_ascii"); }
Value native_hexadecimal_decode_ascii(int argCount, Value* args) { return decode_to_ascii(args, 16, "hexadecimal_decode_ascii"); }
Value native_octal_decode_ascii(int argCount, Value* args) { return decode_to_ascii(args, 8, "octal_decode_ascii"); }

// ==================== String Manipulation ====================

// Fungsi toLowerCase("TEKS")
Value native_toLowerCase(int argCount, Value* args) {
    if (argCount != 1 || args[0].type != VAL_STR) {
        runtime_error("toLowerCase() requires exactly 1 string argument.");
        return make_nil();
    }
    
    int len = strlen(args[0].as.str);
    char* buffer = malloc(len + 1);
    
    // Hajar langsung memori ASCII-nya di level bit!
    for (int i = 0; i < len; i++) {
        buffer[i] = tolower((unsigned char)args[0].as.str[i]);
    }
    buffer[len] = '\0';
    
    Value res = make_gc_str(buffer); // Masuk Garbage Collector agar RAM aman!
    free(buffer);
    return res;
}

// Fungsi toHigherCase("teks")
Value native_toHigherCase(int argCount, Value* args) {
    if (argCount != 1 || args[0].type != VAL_STR) {
        runtime_error("toHigherCase() requires exactly 1 string argument.");
        return make_nil();
    }
    
    int len = strlen(args[0].as.str);
    char* buffer = malloc(len + 1);
    
    // Paksa naikkan nilai ASCII-nya!
    for (int i = 0; i < len; i++) {
        buffer[i] = toupper((unsigned char)args[0].as.str[i]);
    }
    buffer[len] = '\0';
    
    Value res = make_gc_str(buffer); // Masuk Garbage Collector!
    free(buffer);
    return res;
}

// ==================== File System Calls ====================

// NewFile("path/to/file.txt", "Teks yang ditulis")
Value native_NewFile(int argCount, Value* args) {
    if (argCount != 2 || args[0].type != VAL_STR || args[1].type != VAL_STR) {
        runtime_error("NewFile() requires exactly 2 string arguments: (path, content).");
    }
    if (!os_write_file_sys(args[0].as.str, args[1].as.str)) {
        printf("Error: Could not create or write to file '%s'. Permission denied.\n", args[0].as.str);
        return make_bool(false);
    }
    return make_bool(true);
}

// ReadFile("path/to/file.txt")
Value native_ReadFile(int argCount, Value* args) {
    if (argCount != 1 || args[0].type != VAL_STR) {
        runtime_error("ReadFile() requires exactly 1 string argument: (path).");
    }
    char* content = os_read_file_sys(args[0].as.str);
    if (!content) {
        printf("Error: Could not read file '%s'. It may not exist.\n", args[0].as.str);
        return make_str(""); 
    }
    
    // Serahkan string raksasa ini ke pelukan Garbage Collector!
    Value res = make_gc_str(content); 
    free(content);
    return res;
}

// OpenFile("path/to/file.png") -> Membuka file pakai OS Host!
Value native_OpenFile(int argCount, Value* args) {
    if (argCount != 1 || args[0].type != VAL_STR) {
        runtime_error("OpenFile() requires exactly 1 string argument: (path).");
    }
    os_open_sys(args[0].as.str);
    return make_nil();
}

// ==================== Main & Setup ====================
void run_source(const char* source) {
    has_error = false; is_returning = false;
    if (setjmp(error_jump) == 0) {
        tokenize(source); current_tok = 0;
        while (!is_at_end() && !has_error) { Stmt* s = parse_stmt(); if (!has_error) execute(s); }
    }
}

char* read_multiline_input(const char* prompt) {
    int buf_size = 1024; char* buffer = malloc(buf_size); buffer[0] = '\0';
    int brace_count = 0; bool in_string = false; const char* curr_prompt = prompt;
    while (true) {
        char* line = os_readline(curr_prompt);
        if (!line) { printf("\n"); free(buffer); return NULL; }
        if (strlen(line) > 0) os_add_history(line);
        if (strlen(buffer) + strlen(line) + 2 >= buf_size) { buf_size *= 2; buffer = realloc(buffer, buf_size); }
        strcat(buffer, line); strcat(buffer, "\n");
        for (int i = 0; line[i]; i++) {
            if (line[i] == '"') in_string = !in_string;
            if (!in_string) { if (line[i] == '{') brace_count++; if (line[i] == '}') brace_count--; }
        }
        free(line); if (brace_count <= 0) break; curr_prompt = "... ";
    }
    return buffer;
}

char* read_file_system(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) { printf("Could not open file \"%s\".\n", path); exit(74); }
    fseek(file, 0L, SEEK_END); size_t file_size = ftell(file); rewind(file);
    char* buffer = malloc(file_size + 1); size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = '\0'; fclose(file); return buffer;
}

// ==================== OBFUSCATOR COMPILER (Fake Bytecode) ====================

// LAYER 1: Lexical Mutilation (Menghancurkan format kode agar tidak bisa dibaca)
char* minify_source(const char* source) {
    int len = strlen(source);
    char* minified = malloc(len + 1);
    int j = 0;
    bool in_string = false;
    for (int i = 0; i < len; i++) {
        if (source[i] == '"') in_string = !in_string;
        if (!in_string) {
            // Hapus semua komentar secara sadis
            if (source[i] == '/' && i + 1 < len && source[i+1] == '/') {
                while (i < len && source[i] != '\n') i++;
                continue;
            }
            // Hapus baris baru dan tab (kode akan jadi 1 baris panjang berantakan)
            if (source[i] == '\n' || source[i] == '\r' || source[i] == '\t') continue;
        }
        minified[j++] = source[i];
    }
    minified[j] = '\0';
    return minified;
}

void compile_source_to_file(const char* source, const char* out_filepath) {
    FILE* file = fopen(out_filepath, "wb");
    if (!file) { printf("Error: Cannot write to %s\n", out_filepath); return; }
    
    // 1. Cincang kode sumbernya
    char* minified = minify_source(source);
    int len = strlen(minified);
    
    // 2. Header Baru: "VLX\0" (VerseLanguage eXtended - Secure)
    fwrite("VLX\0", sizeof(char), 4, file); 
    
    // 3. Bangkitkan Seed Enkripsi Dinamis dari jam OS
    srand((unsigned int)(os_get_time() * 1000));
    uint8_t seed = rand() % 256;
    fwrite(&seed, sizeof(uint8_t), 1, file);
    fwrite(&len, sizeof(int), 1, file); 
    
    // 4. LAYER 2 & 3: Junk Injection & Rolling XOR
    for (int i = 0; i < len; i++) {
        // Tembakkan 1 sampai 3 byte sampah (Junk Bytes) untuk merusak pola Hex!
        uint8_t junk_count = (rand() % 3) + 1;
        fwrite(&junk_count, sizeof(uint8_t), 1, file);
        for(int j = 0; j < junk_count; j++) {
            uint8_t junk = rand() % 256;
            fwrite(&junk, sizeof(uint8_t), 1, file);
        }
        
        // Mutasikan kunci XOR berdasarkan Seed dan Posisi Index
        uint8_t rolling_key = (seed + i) ^ 0x4B; 
        uint8_t encrypted = (uint8_t)minified[i] ^ rolling_key; 
        fwrite(&encrypted, sizeof(uint8_t), 1, file);
    }
    
    free(minified);
    fclose(file);
}

void run_bytecode_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) { printf("Error: Bytecode file '%s' not found.\n", filepath); exit(74); }
    
    // Validasi Header Super Ketat
    char header[4]; fread(header, sizeof(char), 4, file);
    if (memcmp(header, "VLX\0", 4) != 0) {
        printf("Fatal Error: File '%s' is corrupted or uses weak legacy encryption.\n", filepath);
        fclose(file); exit(74);
    }
    
    // Tarik Seed dan Panjang Asli
    uint8_t seed; fread(&seed, sizeof(uint8_t), 1, file);
    int len; fread(&len, sizeof(int), 1, file);
    char* source = malloc(len + 1);
    
    // Bongkar Enkripsi sambil Membuang Sampah Biner
    for (int i = 0; i < len; i++) {
        // Baca jumlah byte sampah, lalu lompati memorinya! (Bypass Junk)
        uint8_t junk_count; fread(&junk_count, sizeof(uint8_t), 1, file);
        fseek(file, junk_count, SEEK_CUR); 
        
        // Ekstrak byte asli dan dekripsi dengan Rolling Key yang sinkron
        uint8_t decrypted; fread(&decrypted, sizeof(uint8_t), 1, file);
        uint8_t rolling_key = (seed + i) ^ 0x4B;
        source[i] = (char)(decrypted ^ rolling_key); 
    }
    source[len] = '\0';
    fclose(file);
    
    // Hajar AST dengan kode telanjang yang sudah dipulihkan
    run_source(source); 
    free(source);
}

// ==================== Main & Setup ====================
int main(int argc, char** argv) {
    global_env = new_env(NULL); current_env = global_env;
    
    define_var(global_env, "clock", make_native(native_clock), false);
    define_var(global_env, "show", make_native(native_show), false);
    define_var(global_env, "mkinput", make_native(native_mkinput), false);
    define_var(global_env, "wait", make_native(native_wait), false);
    define_var(global_env, "double", make_native(native_double), false);
    define_var(global_env, "NewFile", make_native(native_NewFile), false);
    define_var(global_env, "ReadFile", make_native(native_ReadFile), false);
    define_var(global_env, "OpenFile", make_native(native_OpenFile), false);
    define_var(global_env, "toLowerCase", make_native(native_toLowerCase), false);
    define_var(global_env, "toHigherCase", make_native(native_toHigherCase), false);
    define_var(global_env, "binary_decode", make_native(native_binary_decode), false);
    define_var(global_env, "hexadecimal_decode", make_native(native_hexadecimal_decode), false);
    define_var(global_env, "octal_decode", make_native(native_octal_decode), false);
    define_var(global_env, "binary_decode_ascii", make_native(native_binary_decode_ascii), false);
    define_var(global_env, "hexadecimal_decode_ascii", make_native(native_hexadecimal_decode_ascii), false);
    define_var(global_env, "octal_decode_ascii", make_native(native_octal_decode_ascii), false);
    define_var(global_env, "array", make_native(native_array), false);
    define_var(global_env, "array_push", make_native(native_array_push), false);
    define_var(global_env, "array_get", make_native(native_array_get), false);
    define_var(global_env, "array_len", make_native(native_array_len), false);
    define_var(global_env, "array_remove_first", make_native(native_array_remove_first), false);
    define_var(global_env, "hexadecimal", make_native(native_hexadecimal), false);
    define_var(global_env, "octal", make_native(native_octal), false);
    define_var(global_env, "binary", make_native(native_binary), false);
    define_var(global_env, "clear", make_str("__INTERNAL_CLEAR__"), false);
    define_var(global_env, "PI", make_num(3.14159265359), false);
    define_var(global_env, "VERSION", make_str("2.4-CrossPlatform"), false);
    define_var(global_env, "getCurrentSystem", make_native(native_getCurrentSystem), false);
    define_var(global_env, "cmd", make_native(native_cmd), false);
    define_var(global_env, "int", make_native(native_int), false);
    define_var(global_env, "float", make_native(native_float), false);

    if (argc == 1) {
        printf("VerseLanguage (vl) Interpreter 2.5\n");
        while (true) {
            char* input = read_multiline_input(">> ");
            if (!input) break;
            if (strncmp(input, "exit;", 5) == 0) { free(input); break; }
            run_source(input); free(input);
        }
    } else if (argc >= 2) {
        if (strcmp(argv[1], "compile") == 0) {
            if (argc < 3) { printf("Usage: vl compile <file.vl> [-o <output.vlb>]\n"); return 64; }
            char* input_file = argv[2]; char output_file[1024];
            
            // Logika custom output flag -o
            if (argc == 5 && strcmp(argv[3], "-o") == 0) {
                snprintf(output_file, sizeof(output_file), "%s", argv[4]);
            } else {
                snprintf(output_file, sizeof(output_file), "%s", input_file);
                char* dot = strrchr(output_file, '.');
                if (dot && strcmp(dot, ".vl") == 0) strcpy(dot, ".vlb");
                else strcat(output_file, ".vlb");
            }
            
            char filepath[1024]; snprintf(filepath, sizeof(filepath), "%s", input_file);
            FILE* test_file = fopen(filepath, "rb");
            if (!test_file) {
                int len = strlen(filepath);
                if (len < 3 || strcmp(filepath + len - 3, ".vl") != 0) snprintf(filepath, sizeof(filepath), "%s.vl", argv[2]);
            } else { fclose(test_file); }
            
            char* source = read_file_system(filepath);
            compile_source_to_file(source, output_file);
            free(source);
        } 
        else if (strcmp(argv[1], "-byte") == 0) {
            if (argc < 3) { printf("Usage: vl -byte <file.vlb>\n"); return 64; }
            char filepath[1024]; snprintf(filepath, sizeof(filepath), "%s", argv[2]);
            
            FILE* test_file = fopen(filepath, "rb");
            if (!test_file) {
                int len = strlen(filepath);
                if (len < 4 || strcmp(filepath + len - 4, ".vlb") != 0) snprintf(filepath, sizeof(filepath), "%s.vlb", argv[2]);
            } else { fclose(test_file); }
            
            run_bytecode_file(filepath);
        } 
        else {
            char filepath[1024]; snprintf(filepath, sizeof(filepath), "%s", argv[1]);
            FILE* test_file = fopen(filepath, "rb");
            if (!test_file) {
                int len = strlen(filepath);
                if (len < 3 || strcmp(filepath + len - 3, ".vl") != 0) snprintf(filepath, sizeof(filepath), "%s.vl", argv[1]);
            } else { fclose(test_file); }
            
            char* source = read_file_system(filepath); 
            run_source(source); 
            free(source);
        }
    }
    return 0;
}
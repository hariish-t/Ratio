#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper: Check if character is valid for identifier
static int is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

// Helper: Check if string is a keyword
static TokenType check_keyword(const char *str) {
    // Convert to lowercase for case-insensitive comparison
    char lower[256];
    int i = 0;
    while (str[i] && i < 255) {
        lower[i] = tolower(str[i]);
        i++;
    }
    lower[i] = '\0';

    // Keywords (case insensitive)
    if (strcmp(lower, "start") == 0) return TOKEN_START;
    if (strcmp(lower, "set") == 0) return TOKEN_SET;
    if (strcmp(lower, "echo") == 0) return TOKEN_ECHO;
    if (strcmp(lower, "if") == 0) return TOKEN_IF;
    if (strcmp(lower, "elseif") == 0) return TOKEN_ELSEIF;
    if (strcmp(lower, "else") == 0) return TOKEN_ELSE;
    if (strcmp(lower, "endb") == 0) return TOKEN_ENDB;
    if (strcmp(lower, "for") == 0) return TOKEN_FOR;
    if (strcmp(lower, "while") == 0) return TOKEN_WHILE;
    if (strcmp(lower, "endl") == 0) return TOKEN_ENDL;
    if (strcmp(lower, "break") == 0) return TOKEN_BREAK;
    if (strcmp(lower, "continue") == 0) return TOKEN_CONTINUE;
    if (strcmp(lower, "call") == 0) return TOKEN_CALL;
    if (strcmp(lower, "ret") == 0) return TOKEN_RET;
    if (strcmp(lower, "jmp") == 0) return TOKEN_JMP;
    if (strcmp(lower, "jeq") == 0) return TOKEN_JEQ;
    if (strcmp(lower, "jne") == 0) return TOKEN_JNE;
    if (strcmp(lower, "jgt") == 0) return TOKEN_JGT;
    if (strcmp(lower, "jlt") == 0) return TOKEN_JLT;
    if (strcmp(lower, "jge") == 0) return TOKEN_JGE;
    if (strcmp(lower, "jle") == 0) return TOKEN_JLE;
    if (strcmp(lower, "halt") == 0) return TOKEN_HALT;
    if (strcmp(lower, "type") == 0) return TOKEN_TYPE;
    if (strcmp(lower, "int") == 0) return TOKEN_INT_CAST;
    if (strcmp(lower, "float") == 0) return TOKEN_FLOAT_CAST;
    if (strcmp(lower, "str") == 0) return TOKEN_STR_CAST;
    if (strcmp(lower, "bool") == 0) return TOKEN_BOOL_CAST;
    if (strcmp(lower, "in") == 0) return TOKEN_IN;
    
    // Operations
    if (strcmp(lower, "add") == 0) return TOKEN_ADD;
    if (strcmp(lower, "sub") == 0) return TOKEN_SUB;
    if (strcmp(lower, "mul") == 0) return TOKEN_MUL;
    if (strcmp(lower, "div") == 0) return TOKEN_DIV;
    if (strcmp(lower, "mod") == 0) return TOKEN_MOD;
    if (strcmp(lower, "inc") == 0) return TOKEN_INC;
    if (strcmp(lower, "dec") == 0) return TOKEN_DEC;
    if (strcmp(lower, "concat") == 0) return TOKEN_CONCAT;
    
    // Comparisons
    if (strcmp(lower, "eq") == 0) return TOKEN_EQ;
    if (strcmp(lower, "ne") == 0) return TOKEN_NE;
    if (strcmp(lower, "gt") == 0) return TOKEN_GT;
    if (strcmp(lower, "lt") == 0) return TOKEN_LT;
    if (strcmp(lower, "ge") == 0) return TOKEN_GE;
    if (strcmp(lower, "le") == 0) return TOKEN_LE;
    
    // Logical
    if (strcmp(lower, "and") == 0) return TOKEN_AND;
    if (strcmp(lower, "or") == 0) return TOKEN_OR;
    if (strcmp(lower, "not") == 0) return TOKEN_NOT;
    
    // Boolean literals (case insensitive)
    if (strcmp(lower, "true") == 0) return TOKEN_BOOL_TRUE;
    if (strcmp(lower, "false") == 0) return TOKEN_BOOL_FALSE;

    return TOKEN_IDENTIFIER;
}

// Create token
Token *create_token(TokenType type, const char *value, int line, int column) {
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = line;
    token->column = column;
    return token;
}

// Free token
void free_token(Token *token) {
    if (token) {
        if (token->value) free(token->value);
        free(token);
    }
}

// Get token type name (for debugging)
const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_INT: return "INT";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_BOOL_TRUE: return "TRUE";
        case TOKEN_BOOL_FALSE: return "FALSE";
        case TOKEN_START: return "START";
        case TOKEN_SET: return "SET";
        case TOKEN_ECHO: return "ECHO";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSEIF: return "ELSEIF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_ENDB: return "ENDB";
        case TOKEN_FOR: return "FOR";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_ENDL: return "ENDL";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_CALL: return "CALL";
        case TOKEN_RET: return "RET";
        case TOKEN_JMP: return "JMP";
        case TOKEN_JEQ: return "JEQ";
        case TOKEN_JNE: return "JNE";
        case TOKEN_JGT: return "JGT";
        case TOKEN_JLT: return "JLT";
        case TOKEN_JGE: return "JGE";
        case TOKEN_JLE: return "JLE";
        case TOKEN_HALT: return "HALT";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_INT_CAST: return "INT_CAST";
        case TOKEN_FLOAT_CAST: return "FLOAT_CAST";
        case TOKEN_STR_CAST: return "STR_CAST";
        case TOKEN_BOOL_CAST: return "BOOL_CAST";
        case TOKEN_IN: return "IN";
        case TOKEN_ADD: return "ADD";
        case TOKEN_SUB: return "SUB";
        case TOKEN_MUL: return "MUL";
        case TOKEN_DIV: return "DIV";
        case TOKEN_MOD: return "MOD";
        case TOKEN_INC: return "INC";
        case TOKEN_DEC: return "DEC";
        case TOKEN_CONCAT: return "CONCAT";
        case TOKEN_EQ: return "EQ";
        case TOKEN_NE: return "NE";
        case TOKEN_GT: return "GT";
        case TOKEN_LT: return "LT";
        case TOKEN_GE: return "GE";
        case TOKEN_LE: return "LE";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COLON: return "COLON";
        case TOKEN_UNDERSCORE: return "UNDERSCORE";
        case TOKEN_DOLLAR: return "DOLLAR";
        case TOKEN_ELLIPSIS: return "ELLIPSIS";
        case TOKEN_LABEL: return "LABEL";
        case TOKEN_NEWLINE: return "NEWLINE";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

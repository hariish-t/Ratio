#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    // Literals
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_BOOL_TRUE,
    TOKEN_BOOL_FALSE,
    
    // Keywords
    TOKEN_START,
    TOKEN_SET,
    TOKEN_ECHO,
    TOKEN_IF,
    TOKEN_ELSEIF,
    TOKEN_ELSE,
    TOKEN_ENDB,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_ENDL,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_CALL,
    TOKEN_RET,
    TOKEN_JMP,
    TOKEN_JEQ,
    TOKEN_JNE,
    TOKEN_JGT,
    TOKEN_JLT,
    TOKEN_JGE,
    TOKEN_JLE,
    TOKEN_HALT,
    TOKEN_TYPE,
    TOKEN_INT_CAST,
    TOKEN_FLOAT_CAST,
    TOKEN_STR_CAST,
    TOKEN_BOOL_CAST,
    TOKEN_IN,
    
    // Operations
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_CONCAT,
    
    // Comparisons
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_GE,
    TOKEN_LE,
    
    // Logical
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    
    // Symbols
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COLON,
    TOKEN_UNDERSCORE,
    TOKEN_DOLLAR,
    TOKEN_ELLIPSIS,      // ...
    
    // Special
    TOKEN_LABEL,         // .labelname
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *value;         // String representation
    int line;            // Line number
    int column;          // Column number
} Token;

// Function to create a new token
Token *create_token(TokenType type, const char *value, int line, int column);

// Function to free token memory
void free_token(Token *token);

// Function to get token type name (for debugging)
const char *token_type_name(TokenType type);

#endif

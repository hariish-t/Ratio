
#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    const char *source;   // Source code
    int position;         // Current position
    int line;             // Current line
    int column;           // Current column
    char current_char;    // Current character
} Lexer;

// Initialize lexer
Lexer *create_lexer(const char *source);

// Free lexer
void free_lexer(Lexer *lexer);

// Get next token
Token *get_next_token(Lexer *lexer);

// Tokenize entire source (returns array of tokens)
Token **tokenize(const char *source, int *token_count);

#endif

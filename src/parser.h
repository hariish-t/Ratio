#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"

typedef struct {
    Token **tokens;
    int token_count;
    int current;
} Parser;

// Create parser
Parser *create_parser(Token **tokens, int token_count);

// Free parser
void free_parser(Parser *parser);

// Parse tokens into AST
ASTNode *parse(Token **tokens, int token_count);

// Helper functions
Token *current_token(Parser *parser);
Token *peek_token(Parser *parser, int offset);
void advance_parser(Parser *parser);
int match(Parser *parser, TokenType type);
int match_any(Parser *parser, TokenType *types, int count);
Token *consume(Parser *parser, TokenType type, const char *error_message);

#endif

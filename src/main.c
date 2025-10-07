#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "ast.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename.ratio>\n", argv[0]);
        return 1;
    }

    // Read source file
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *source = malloc(file_size + 1);
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);

    printf("=== RATIO INTERPRETER v1.0 ===\n");
    printf("Source file: %s\n\n", argv[1]);

    // Tokenize
    int token_count = 0;
    Token **tokens = tokenize(source, &token_count);
    printf("Tokenized: %d tokens\n\n", token_count);

    // Parse
    printf("=== PARSING ===\n");
    ASTNode *ast = parse(tokens, token_count);
    printf("Parse complete!\n\n");

    // Print AST (for debugging)
printf("=== AST ===\n");

// Debug: Check what's in the program before printing
if (ast && ast->type == AST_PROGRAM) {
    printf("Program has %d statements\n", ast->data.program.statement_count);
    for (int i = 0; i < ast->data.program.statement_count; i++) {
        ASTNode *stmt = ast->data.program.statements[i];
        if (stmt == NULL) {
            printf("  Statement %d: NULL\n", i);
        } else {
            printf("  Statement %d: type=%d\n", i, stmt->type);
        }
    }
}

printf("\nNow printing full AST:\n");
print_ast(ast, 0);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "token.h"

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

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read entire file
    char *source = malloc(file_size + 1);
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);

    printf("=== RATIO INTERPRETER v1.0 ===\n");
    printf("Source file: %s\n\n", argv[1]);

    // Tokenize
    int token_count = 0;
    Token **tokens = tokenize(source, &token_count);

    // Print tokens (for debugging)
    printf("=== TOKENS ===\n");
    for (int i = 0; i < token_count; i++) {
        printf("[%d:%d] %s", 
               tokens[i]->line, 
               tokens[i]->column,
               token_type_name(tokens[i]->type));
        
        if (tokens[i]->value && strlen(tokens[i]->value) > 0) {
            printf(" '%s'", tokens[i]->value);
        }
        printf("\n");
    }

    // Cleanup
    for (int i = 0; i < token_count; i++) {
        free_token(tokens[i]);
    }
    free(tokens);
    free(source);

    printf("\n=== DONE ===\n");
    return 0;
}

#ifndef AST_H
#define AST_H

#include "token.h"

// Node types
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_LABEL,
    AST_ASSIGNMENT,
    AST_OPERATION,
    AST_IF_STATEMENT,
    AST_FOR_LOOP,
    AST_WHILE_LOOP,
    AST_FUNCTION_CALL,
    AST_RETURN,
    AST_JUMP,
    AST_ECHO,
    AST_BREAK,
    AST_CONTINUE,
    AST_HALT,
    AST_TYPE_CHECK,
    AST_TYPE_CAST,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_IDENTIFIER,
    AST_LITERAL_INT,
    AST_LITERAL_FLOAT,
    AST_LITERAL_STRING,
    AST_LITERAL_BOOL,
    AST_ARRAY,
    AST_ARRAY_ACCESS,
    AST_PROPERTY_ACCESS,
    AST_INPUT
} ASTNodeType;

// Forward declaration
typedef struct ASTNode ASTNode;

// AST Node structure
struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    union {
        // Program: list of statements
        struct {
            ASTNode **statements;
            int statement_count;
        } program;
        
        // Function definition
        struct {
            char *name;              // .funcName
            char **parameters;       // parameter names
            int param_count;
            ASTNode **body;          // function body statements
            int body_count;
        } function;
        
        // Label
        struct {
            char *name;              // .labelName
        } label;
        
        // Assignment: set x,10 or set x eq 10
        struct {
            char *variable;
            ASTNode *value;
        } assignment;
        
        // Binary operation: add x,y eq z
        struct {
            TokenType op;            // ADD, SUB, MUL, DIV, MOD, etc.
            ASTNode *left;
            ASTNode *right;
            char *result;            // result variable (optional)
        } binary_op;
        
        // Unary operation: inc x, dec x
        struct {
            TokenType op;            // INC, DEC
            char *variable;
            ASTNode *amount;         // optional: inc x,5
        } unary_op;
        
        // If statement
        struct {
            ASTNode *condition;
            ASTNode **then_body;
            int then_count;
            ASTNode **else_body;
            int else_count;
        } if_stmt;
        
        // For loop
        struct {
            char *variable;
            ASTNode *start;
            ASTNode *end;
            ASTNode *step;           // optional
            ASTNode **body;
            int body_count;
            char *label;             // optional: for i (1...10) _myloop
        } for_loop;
        
        // While loop
        struct {
            ASTNode *condition;
            ASTNode **body;
            int body_count;
            char *label;             // optional
        } while_loop;
        
        // Function call: call .func(a,b) eq result
        struct {
            char *function_name;
            ASTNode **arguments;
            int arg_count;
            char **result_vars;      // for multiple returns
            int result_count;
        } function_call;
        
        // Return statement
        struct {
            ASTNode **values;
            int value_count;
        } return_stmt;
        
        // Jump: jmp .label, jeq x,y .label
        struct {
            TokenType jump_type;     // JMP, JEQ, JNE, etc.
            ASTNode *left;           // for conditional jumps
            ASTNode *right;
            char *target_label;
        } jump;
        
        // Echo statement
        struct {
            ASTNode **expressions;
            int expr_count;
        } echo;
        
        // Break/Continue
        struct {
            char *label;             // optional: break outer
        } break_continue;
        
        // Halt
        struct {
            ASTNode *message;        // optional message
        } halt;
        
        // Type check: type x
        struct {
            char *variable;
            char *result_var;        // optional: type t eq x
        } type_check;
        
        // Type cast: int x eq y
        struct {
            TokenType target_type;   // INT_CAST, FLOAT_CAST, etc.
            ASTNode *value;
            char *result_var;
        } type_cast;
        
        // Identifier
        struct {
            char *name;
        } identifier;
        
        // Literals
        struct {
            int value;
        } int_literal;
        
        struct {
            double value;
        } float_literal;
        
        struct {
            char *value;
        } string_literal;
        
        struct {
            int value;               // 0 or 1
        } bool_literal;
        
        // Array: {1,2,3}
        struct {
            ASTNode **elements;
            int element_count;
        } array;
        
        // Array access: arr[0]
        struct {
            char *array_name;
            ASTNode *index;
        } array_access;
        
        // Property access: arr.len
        struct {
            char *object_name;
            char *property;
        } property_access;
        
        // Input: $
        struct {
            ASTNode *prompt;         // optional prompt string
        } input;
    } data;
};

// Create AST nodes
ASTNode *create_ast_node(ASTNodeType type, int line, int column);
void free_ast_node(ASTNode *node);
void print_ast(ASTNode *node, int indent);

#endif

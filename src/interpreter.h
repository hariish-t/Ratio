#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

// Value types
typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOL,
    VAL_ARRAY,
    VAL_NULL
} ValueType;

// Runtime value
typedef struct Value {
    ValueType type;
    union {
        int int_val;
        double float_val;
        char *string_val;
        int bool_val;
        struct {
            struct Value **elements;
            int count;
        } array_val;
    } data;
} Value;

// Variable storage (simple hash map)
#define VAR_TABLE_SIZE 256

typedef struct Variable {
    char *name;
    Value *value;
    struct Variable *next;  // for collision chaining
} Variable;

typedef struct {
    Variable *table[VAR_TABLE_SIZE];
} Environment;

// Create/destroy values
Value *create_value(ValueType type);
Value *create_int_value(int val);
Value *create_float_value(double val);
Value *create_string_value(const char *val);
Value *create_bool_value(int val);
Value *create_array_value(Value **elements, int count);
void free_value(Value *val);
Value *copy_value(Value *val);

// Environment functions
Environment *create_environment();
void free_environment(Environment *env);
void set_variable(Environment *env, const char *name, Value *value);
Value *get_variable(Environment *env, const char *name);

// Interpreter
void interpret(ASTNode *ast);
// Value *eval_node(ASTNode *node, Environment *env);

// Debug
void print_value(Value *val);
const char *value_type_name(ValueType type);

#endif

#define _POSIX_C_SOURCE 200809L

#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==================== VALUE FUNCTIONS ====================

Value *create_value(ValueType type) {
    Value *val = malloc(sizeof(Value));
    val->type = type;
    memset(&val->data, 0, sizeof(val->data));
    return val;
}

Value *create_int_value(int val) {
    Value *v = create_value(VAL_INT);
    v->data.int_val = val;
    return v;
}

Value *create_float_value(double val) {
    Value *v = create_value(VAL_FLOAT);
    v->data.float_val = val;
    return v;
}

Value *create_string_value(const char *val) {
    Value *v = create_value(VAL_STRING);
    v->data.string_val = strdup(val);
    return v;
}

Value *create_bool_value(int val) {
    Value *v = create_value(VAL_BOOL);
    v->data.bool_val = val ? 1 : 0;
    return v;
}

Value *create_array_value(Value **elements, int count) {
    Value *v = create_value(VAL_ARRAY);
    v->data.array_val.elements = malloc(sizeof(Value*) * count);
    v->data.array_val.count = count;
    for (int i = 0; i < count; i++) {
        v->data.array_val.elements[i] = copy_value(elements[i]);
    }
    return v;
}

void free_value(Value *val) {
    if (!val) return;
    
    if (val->type == VAL_STRING && val->data.string_val) {
        free(val->data.string_val);
    } else if (val->type == VAL_ARRAY) {
        for (int i = 0; i < val->data.array_val.count; i++) {
            free_value(val->data.array_val.elements[i]);
        }
        free(val->data.array_val.elements);
    }
    
    free(val);
}

Value *copy_value(Value *val) {
    if (!val) return NULL;
    
    switch (val->type) {
        case VAL_INT:
            return create_int_value(val->data.int_val);
        case VAL_FLOAT:
            return create_float_value(val->data.float_val);
        case VAL_STRING:
            return create_string_value(val->data.string_val);
        case VAL_BOOL:
            return create_bool_value(val->data.bool_val);
        case VAL_ARRAY:
            return create_array_value(val->data.array_val.elements, val->data.array_val.count);
        default:
            return create_value(VAL_NULL);
    }
}

void print_value(Value *val) {
    if (!val) {
        printf("(null)");
        return;
    }
    
    switch (val->type) {
        case VAL_INT:
            printf("%d", val->data.int_val);
            break;
        case VAL_FLOAT:
            printf("%f", val->data.float_val);
            break;
        case VAL_STRING:
            printf("%s", val->data.string_val);
            break;
        case VAL_BOOL:
            printf("%s", val->data.bool_val ? "true" : "false");
            break;
        case VAL_ARRAY:
            printf("{");
            for (int i = 0; i < val->data.array_val.count; i++) {
                print_value(val->data.array_val.elements[i]);
                if (i < val->data.array_val.count - 1) printf(", ");
            }
            printf("}");
            break;
        case VAL_NULL:
            printf("null");
            break;
    }
}

const char *value_type_name(ValueType type) {
    switch (type) {
        case VAL_INT: return "int";
        case VAL_FLOAT: return "float";
        case VAL_STRING: return "string";
        case VAL_BOOL: return "bool";
        case VAL_ARRAY: return "array";
        case VAL_NULL: return "null";
        default: return "unknown";
    }
}

// ==================== ENVIRONMENT (VARIABLE STORAGE) ====================

// Simple hash function
static unsigned int hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % VAR_TABLE_SIZE;
}

Environment *create_environment() {
    Environment *env = malloc(sizeof(Environment));
    memset(env->table, 0, sizeof(env->table));
    return env;
}

void free_environment(Environment *env) {
    if (!env) return;
    
    for (int i = 0; i < VAR_TABLE_SIZE; i++) {
        Variable *var = env->table[i];
        while (var) {
            Variable *next = var->next;
            free(var->name);
            free_value(var->value);
            free(var);
            var = next;
        }
    }
    free(env);
}

void set_variable(Environment *env, const char *name, Value *value) {
    unsigned int index = hash(name);
    
    // Check if variable already exists
    Variable *var = env->table[index];
    while (var) {
        if (strcmp(var->name, name) == 0) {
            // Update existing variable
            free_value(var->value);
            var->value = copy_value(value);
            return;
        }
        var = var->next;
    }
    
    // Create new variable
    Variable *new_var = malloc(sizeof(Variable));
    new_var->name = strdup(name);
    new_var->value = copy_value(value);
    new_var->next = env->table[index];
    env->table[index] = new_var;
}

Value *get_variable(Environment *env, const char *name) {
    unsigned int index = hash(name);
    
    Variable *var = env->table[index];
    while (var) {
        if (strcmp(var->name, name) == 0) {
            return var->value;
        }
        var = var->next;
    }
    
    fprintf(stderr, "Runtime Error: Undefined variable '%s'\n", name);
    return create_value(VAL_NULL);
}

// ==================== EVALUATION / EXECUTION ====================

// Forward declaration
static Value *eval_node(ASTNode *node, Environment *env);

// Evaluate literal values
static Value *eval_literal(ASTNode *node) {
    switch (node->type) {
        case AST_LITERAL_INT:
            return create_int_value(node->data.int_literal.value);
        
        case AST_LITERAL_FLOAT:
            return create_float_value(node->data.float_literal.value);
        
        case AST_LITERAL_STRING:
            return create_string_value(node->data.string_literal.value);
        
        case AST_LITERAL_BOOL:
            return create_bool_value(node->data.bool_literal.value);
        
        default:
            return create_value(VAL_NULL);
    }
}

// Evaluate identifier (variable lookup)
static Value *eval_identifier(ASTNode *node, Environment *env) {
    return get_variable(env, node->data.identifier.name);
}

// Evaluate binary operation
static Value *eval_binary_op(ASTNode *node, Environment *env) {
    Value *left = eval_node(node->data.binary_op.left, env);
    Value *right = eval_node(node->data.binary_op.right, env);
    
    if (!left || !right) {
        free_value(left);
        free_value(right);
        return create_value(VAL_NULL);
    }
    
    Value *result = NULL;
    
    switch (node->data.binary_op.op) {
        case TOKEN_ADD:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_int_value(left->data.int_val + right->data.int_val);
            } else if (left->type == VAL_FLOAT || right->type == VAL_FLOAT) {
                double l = (left->type == VAL_FLOAT) ? left->data.float_val : left->data.int_val;
                double r = (right->type == VAL_FLOAT) ? right->data.float_val : right->data.int_val;
                result = create_float_value(l + r);
            }
            break;
        
        case TOKEN_SUB:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_int_value(left->data.int_val - right->data.int_val);
            } else if (left->type == VAL_FLOAT || right->type == VAL_FLOAT) {
                double l = (left->type == VAL_FLOAT) ? left->data.float_val : left->data.int_val;
                double r = (right->type == VAL_FLOAT) ? right->data.float_val : right->data.int_val;
                result = create_float_value(l - r);
            }
            break;
        
        case TOKEN_MUL:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_int_value(left->data.int_val * right->data.int_val);
            } else if (left->type == VAL_FLOAT || right->type == VAL_FLOAT) {
                double l = (left->type == VAL_FLOAT) ? left->data.float_val : left->data.int_val;
                double r = (right->type == VAL_FLOAT) ? right->data.float_val : right->data.int_val;
                result = create_float_value(l * r);
            }
            break;
        
        case TOKEN_DIV:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                if (right->data.int_val == 0) {
                    fprintf(stderr, "Runtime Error: Division by zero\n");
                    result = create_value(VAL_NULL);
                } else {
                    result = create_int_value(left->data.int_val / right->data.int_val);
                }
            } else if (left->type == VAL_FLOAT || right->type == VAL_FLOAT) {
                double l = (left->type == VAL_FLOAT) ? left->data.float_val : left->data.int_val;
                double r = (right->type == VAL_FLOAT) ? right->data.float_val : right->data.int_val;
                if (r == 0.0) {
                    fprintf(stderr, "Runtime Error: Division by zero\n");
                    result = create_value(VAL_NULL);
                } else {
                    result = create_float_value(l / r);
                }
            }
            break;
        
        case TOKEN_MOD:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                if (right->data.int_val == 0) {
                    fprintf(stderr, "Runtime Error: Modulo by zero\n");
                    result = create_value(VAL_NULL);
                } else {
                    result = create_int_value(left->data.int_val % right->data.int_val);
                }
            }
            break;
        
        // Comparison operators
        case TOKEN_EQ:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_bool_value(left->data.int_val == right->data.int_val);
            } else if (left->type == VAL_STRING && right->type == VAL_STRING) {
                result = create_bool_value(strcmp(left->data.string_val, right->data.string_val) == 0);
            }
            break;
        
        case TOKEN_NE:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_bool_value(left->data.int_val != right->data.int_val);
            }
            break;
        
        case TOKEN_GT:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_bool_value(left->data.int_val > right->data.int_val);
            }
            break;
        
        case TOKEN_LT:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_bool_value(left->data.int_val < right->data.int_val);
            }
            break;
        
        case TOKEN_GE:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_bool_value(left->data.int_val >= right->data.int_val);
            }
            break;
        
        case TOKEN_LE:
            if (left->type == VAL_INT && right->type == VAL_INT) {
                result = create_bool_value(left->data.int_val <= right->data.int_val);
            }
            break;
        
        // Logical operators
        case TOKEN_AND:
            if (left->type == VAL_BOOL && right->type == VAL_BOOL) {
                result = create_bool_value(left->data.bool_val && right->data.bool_val);
            }
            break;
        
        case TOKEN_OR:
            if (left->type == VAL_BOOL && right->type == VAL_BOOL) {
                result = create_bool_value(left->data.bool_val || right->data.bool_val);
            }
            break;
        
        default:
            result = create_value(VAL_NULL);
            break;
    }
    
    free_value(left);
    free_value(right);
    
    // If result variable specified, store it
    if (result && node->data.binary_op.result) {
        set_variable(env, node->data.binary_op.result, result);
    }
    
    return result ? result : create_value(VAL_NULL);
}

// Execute assignment
static Value *exec_assignment(ASTNode *node, Environment *env) {
    Value *val = eval_node(node->data.assignment.value, env);
    set_variable(env, node->data.assignment.variable, val);
    free_value(val);  // Free the temp value (set_variable made a copy)
    return create_value(VAL_NULL);  // Return a throwaway null
}

// Execute echo statement
static Value *exec_echo(ASTNode *node, Environment *env) {
    for (int i = 0; i < node->data.echo.expr_count; i++) {
        Value *val = eval_node(node->data.echo.expressions[i], env);
        print_value(val);
        if (i < node->data.echo.expr_count - 1) {
            printf(" ");
        }
        free_value(val);
    }
    printf("\n");
    return create_value(VAL_NULL);  // Return a throwaway null
}
// Execute array literal
static Value *exec_array(ASTNode *node, Environment *env) {
    Value **elements = malloc(sizeof(Value*) * node->data.array.element_count);
    
    for (int i = 0; i < node->data.array.element_count; i++) {
        elements[i] = eval_node(node->data.array.elements[i], env);
    }
    
    Value *result = create_array_value(elements, node->data.array.element_count);
    
    // Free temporary elements
    for (int i = 0; i < node->data.array.element_count; i++) {
        free_value(elements[i]);
    }
    free(elements);
    
    return result;
}

// Execute array access
static Value *exec_array_access(ASTNode *node, Environment *env) {
    Value *array = get_variable(env, node->data.array_access.array_name);
    Value *index_val = eval_node(node->data.array_access.index, env);
    
    if (array->type != VAL_ARRAY) {
        fprintf(stderr, "Runtime Error: Not an array\n");
        free_value(index_val);
        return create_value(VAL_NULL);
    }
    
    if (index_val->type != VAL_INT) {
        fprintf(stderr, "Runtime Error: Array index must be integer\n");
        free_value(index_val);
        return create_value(VAL_NULL);
    }
    
    int index = index_val->data.int_val;
    
    // Handle negative indexing
    if (index < 0) {
        index = array->data.array_val.count + index;
    }
    
    if (index < 0 || index >= array->data.array_val.count) {
        fprintf(stderr, "Runtime Error: Array index out of bounds\n");
        free_value(index_val);
        return create_value(VAL_NULL);
    }
    
    Value *result = copy_value(array->data.array_val.elements[index]);
    free_value(index_val);
    return result;
}

// Main eval function
static Value *eval_node(ASTNode *node, Environment *env) {
    if (!node) return create_value(VAL_NULL);
    
    switch (node->type) {
        case AST_LITERAL_INT:
        case AST_LITERAL_FLOAT:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOL:
            return eval_literal(node);
        
        case AST_IDENTIFIER:
            return eval_identifier(node, env);
        
        case AST_BINARY_OP:
            return eval_binary_op(node, env);
        
        case AST_ASSIGNMENT:
            return exec_assignment(node, env);
        
        case AST_ECHO:
            return exec_echo(node, env);
        
        case AST_ARRAY:
            return exec_array(node, env);
        
        case AST_ARRAY_ACCESS:
            return exec_array_access(node, env);
        
        default:
            fprintf(stderr, "Runtime Error: Unimplemented node type %d\n", node->type);
            return create_value(VAL_NULL);
    }
}

// Main interpreter entry point
void interpret(ASTNode *ast) {
    if (!ast || ast->type != AST_PROGRAM) {
        fprintf(stderr, "Runtime Error: Invalid AST\n");
        return;
    }
    
    Environment *env = create_environment();
    
   // Execute all statements
    for (int i = 0; i < ast->data.program.statement_count; i++) {
        Value *result = eval_node(ast->data.program.statements[i], env);
        free_value(result);  // This frees the throwaway null
    }
    
    // free_environment(env);
}

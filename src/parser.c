#define _POSIX_C_SOURCE 200809L

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create parser
Parser *create_parser(Token **tokens, int token_count) {
    Parser *parser = malloc(sizeof(Parser));
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    return parser;
}

// Free parser
void free_parser(Parser *parser) {
    if (parser) {
        free(parser);
    }
}

// Get current token
Token *current_token(Parser *parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current];
    }
    return parser->tokens[parser->token_count - 1]; // EOF
}

// Peek ahead
Token *peek_token(Parser *parser, int offset) {
    int pos = parser->current + offset;
    if (pos < parser->token_count) {
        return parser->tokens[pos];
    }
    return parser->tokens[parser->token_count - 1]; // EOF
}

// Move to next token
void advance_parser(Parser *parser) {
    if (parser->current < parser->token_count - 1) {
        parser->current++;
    }
}

// Check if current token matches type
int match(Parser *parser, TokenType type) {
    return current_token(parser)->type == type;
}

// Check if current token matches any of the types
int match_any(Parser *parser, TokenType *types, int count) {
    for (int i = 0; i < count; i++) {
        if (match(parser, types[i])) {
            return 1;
        }
    }
    return 0;
}

// Consume token of expected type
Token *consume(Parser *parser, TokenType type, const char *error_message) {
    Token *token = current_token(parser);
    if (token->type != type) {
        fprintf(stderr, "Parse Error [%d:%d]: %s (got %s)\n",
                token->line, token->column,
                error_message,
                token_type_name(token->type));
        exit(1);
    }
    advance_parser(parser);
    return token;
}

// Create AST node
ASTNode *create_ast_node(ASTNodeType type, int line, int column) {
    ASTNode *node = malloc(sizeof(ASTNode));
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    return node;
}

// Forward declarations for parsing functions
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_primary(Parser *parser);

// ==================== EXPRESSION PARSING ====================

// Parse primary expression (literals, identifiers, arrays, etc.)
static ASTNode *parse_primary(Parser *parser) {
    Token *token = current_token(parser);
    
    // Integer literal
    if (match(parser, TOKEN_INT)) {
        ASTNode *node = create_ast_node(AST_LITERAL_INT, token->line, token->column);
        node->data.int_literal.value = atoi(token->value);
        advance_parser(parser);
        return node;
    }
    
    // Float literal
    if (match(parser, TOKEN_FLOAT)) {
        ASTNode *node = create_ast_node(AST_LITERAL_FLOAT, token->line, token->column);
        node->data.float_literal.value = atof(token->value);
        advance_parser(parser);
        return node;
    }
    
    // String literal
    if (match(parser, TOKEN_STRING)) {
        ASTNode *node = create_ast_node(AST_LITERAL_STRING, token->line, token->column);
        node->data.string_literal.value = strdup(token->value);
        advance_parser(parser);
        return node;
    }
    
    // Boolean literals
    if (match(parser, TOKEN_BOOL_TRUE)) {
        ASTNode *node = create_ast_node(AST_LITERAL_BOOL, token->line, token->column);
        node->data.bool_literal.value = 1;
        advance_parser(parser);
        return node;
    }
    
    if (match(parser, TOKEN_BOOL_FALSE)) {
        ASTNode *node = create_ast_node(AST_LITERAL_BOOL, token->line, token->column);
        node->data.bool_literal.value = 0;
        advance_parser(parser);
        return node;
    }
    
    // Input: $
    if (match(parser, TOKEN_DOLLAR)) {
        ASTNode *node = create_ast_node(AST_INPUT, token->line, token->column);
        advance_parser(parser);
        
        // Optional prompt string
        if (match(parser, TOKEN_STRING)) {
            node->data.input.prompt = parse_primary(parser);
        } else {
            node->data.input.prompt = NULL;
        }
        return node;
    }
    
    // Array literal: {1,2,3}
    if (match(parser, TOKEN_LBRACE)) {
        ASTNode *node = create_ast_node(AST_ARRAY, token->line, token->column);
        advance_parser(parser); // skip {
        
        // Parse array elements
        ASTNode **elements = malloc(sizeof(ASTNode*) * 100);
        int count = 0;
        
        while (!match(parser, TOKEN_RBRACE) && !match(parser, TOKEN_EOF)) {
            elements[count++] = parse_expression(parser);
            if (match(parser, TOKEN_COMMA)) {
                advance_parser(parser);
            }
        }
        
        consume(parser, TOKEN_RBRACE, "Expected '}' after array elements");
        
        node->data.array.elements = elements;
        node->data.array.element_count = count;
        return node;
    }
    
    // Identifier (variable or function call or array access)
    if (match(parser, TOKEN_IDENTIFIER)) {
        char *name = strdup(token->value);
        advance_parser(parser);
        
        // Array access: arr[0]
        if (match(parser, TOKEN_LBRACKET)) {
            ASTNode *node = create_ast_node(AST_ARRAY_ACCESS, token->line, token->column);
            advance_parser(parser); // skip [
            node->data.array_access.array_name = name;
            node->data.array_access.index = parse_expression(parser);
            consume(parser, TOKEN_RBRACKET, "Expected ']' after array index");
            return node;
        }
        
        // Property access: arr.len
        if (match(parser, TOKEN_DOT)) {
            advance_parser(parser); // skip .
            Token *prop = consume(parser, TOKEN_IDENTIFIER, "Expected property name after '.'");
            ASTNode *node = create_ast_node(AST_PROPERTY_ACCESS, token->line, token->column);
            node->data.property_access.object_name = name;
            node->data.property_access.property = strdup(prop->value);
            return node;
        }
        
        // Just an identifier
        ASTNode *node = create_ast_node(AST_IDENTIFIER, token->line, token->column);
        node->data.identifier.name = name;
        return node;
    }
    
    // Parenthesized expression
    if (match(parser, TOKEN_LPAREN)) {
        advance_parser(parser);
        ASTNode *expr = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    fprintf(stderr, "Parse Error [%d:%d]: Unexpected token %s\n",
            token->line, token->column, token_type_name(token->type));
    exit(1);
}

// Parse expression (operations, comparisons, etc.)
static ASTNode *parse_expression(Parser *parser) {
    Token *token = current_token(parser);
    
    // Type cast: int x, float y, str z, bool b
    if (match(parser, TOKEN_INT_CAST) || match(parser, TOKEN_FLOAT_CAST) ||
        match(parser, TOKEN_STR_CAST) || match(parser, TOKEN_BOOL_CAST)) {
        TokenType cast_type = token->type;
        advance_parser(parser);
        
        ASTNode *value = parse_expression(parser);
        
        // Check for result: int x eq y
        char *result_var = NULL;
        if (match(parser, TOKEN_EQ)) {
            advance_parser(parser);
            Token *var = consume(parser, TOKEN_IDENTIFIER, "Expected variable name after 'eq'");
            result_var = strdup(var->value);
        }
        
        ASTNode *node = create_ast_node(AST_TYPE_CAST, token->line, token->column);
        node->data.type_cast.target_type = cast_type;
        node->data.type_cast.value = value;
        node->data.type_cast.result_var = result_var;
        return node;
    }
    
    // Binary operations: add, sub, mul, div, mod
    if (match(parser, TOKEN_ADD) || match(parser, TOKEN_SUB) || 
        match(parser, TOKEN_MUL) || match(parser, TOKEN_DIV) || 
        match(parser, TOKEN_MOD)) {
        TokenType op = token->type;
        advance_parser(parser);
        
        ASTNode *left = parse_expression(parser);
        consume(parser, TOKEN_COMMA, "Expected ',' after first operand");
        ASTNode *right = parse_expression(parser);
        
        // Optional result: add x,y eq z
        char *result = NULL;
        if (match(parser, TOKEN_EQ)) {
            advance_parser(parser);
            Token *res = consume(parser, TOKEN_IDENTIFIER, "Expected variable name after 'eq'");
            result = strdup(res->value);
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, token->line, token->column);
        node->data.binary_op.op = op;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        node->data.binary_op.result = result;
        return node;
    }
    
    // Comparison operations (for conditions)
    ASTNode *left = parse_primary(parser);
    
    // Check for comparison operators
    if (match(parser, TOKEN_EQ) || match(parser, TOKEN_NE) ||
        match(parser, TOKEN_GT) || match(parser, TOKEN_LT) ||
        match(parser, TOKEN_GE) || match(parser, TOKEN_LE)) {
        TokenType op = current_token(parser)->type;
        int line = current_token(parser)->line;
        int col = current_token(parser)->column;
        advance_parser(parser);
        
        ASTNode *right = parse_expression(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, line, col);
        node->data.binary_op.op = op;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        node->data.binary_op.result = NULL;
        return node;
    }
    
    // Logical operators: and, or
    if (match(parser, TOKEN_AND) || match(parser, TOKEN_OR)) {
        TokenType op = current_token(parser)->type;
        int line = current_token(parser)->line;
        int col = current_token(parser)->column;
        advance_parser(parser);
        
        ASTNode *right = parse_expression(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, line, col);
        node->data.binary_op.op = op;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        node->data.binary_op.result = NULL;
        return node;
    }
    
    return left;
}

// ==================== STATEMENT PARSING ====================

// Parse assignment: set x,10 or set x eq 10
static ASTNode *parse_assignment(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'set'
    
    Token *var = consume(parser, TOKEN_IDENTIFIER, "Expected variable name after 'set'");
    
    // Expect comma or 'eq'
    if (!match(parser, TOKEN_COMMA) && !match(parser, TOKEN_EQ)) {
        fprintf(stderr, "Parse Error: Expected ',' or 'eq' after variable name\n");
        exit(1);
    }
    advance_parser(parser);
    
    ASTNode *value = parse_expression(parser);
    
    ASTNode *node = create_ast_node(AST_ASSIGNMENT, token->line, token->column);
    node->data.assignment.variable = strdup(var->value);
    node->data.assignment.value = value;
    return node;
}

// Parse echo: echo "text" var "more"
static ASTNode *parse_echo(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'echo'
    
    ASTNode *node = create_ast_node(AST_ECHO, token->line, token->column);
    ASTNode **expressions = malloc(sizeof(ASTNode*) * 100);
    int count = 0;
    
    // Parse all expressions until newline or EOF
    while (!match(parser, TOKEN_NEWLINE) && !match(parser, TOKEN_EOF)) {
        expressions[count++] = parse_expression(parser);
    }
    
    node->data.echo.expressions = expressions;
    node->data.echo.expr_count = count;
    return node;
}

// Parse if statement
static ASTNode *parse_if(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'if'
    
    // Optional parentheses
    int has_parens = 0;
    if (match(parser, TOKEN_LPAREN)) {
        has_parens = 1;
        advance_parser(parser);
    }
    
    ASTNode *condition = parse_expression(parser);
    
    if (has_parens) {
        consume(parser, TOKEN_RPAREN, "Expected ')' after condition");
    }
    
    // Parse then body
    ASTNode **then_body = malloc(sizeof(ASTNode*) * 100);
    int then_count = 0;
    
    while (!match(parser, TOKEN_ELSEIF) && !match(parser, TOKEN_ELSE) && 
           !match(parser, TOKEN_ENDB) && !match(parser, TOKEN_EOF)) {
        then_body[then_count++] = parse_statement(parser);
    }
    
    // Parse else/elseif
    ASTNode **else_body = malloc(sizeof(ASTNode*) * 100);
    int else_count = 0;
    
    if (match(parser, TOKEN_ELSEIF)) {
        // TODO: Handle elseif as nested if
        else_body[else_count++] = parse_if(parser);
    } else if (match(parser, TOKEN_ELSE)) {
        advance_parser(parser);
        while (!match(parser, TOKEN_ENDB) && !match(parser, TOKEN_EOF)) {
            else_body[else_count++] = parse_statement(parser);
        }
    }
    
    consume(parser, TOKEN_ENDB, "Expected 'endb' to close if statement");
    
    ASTNode *node = create_ast_node(AST_IF_STATEMENT, token->line, token->column);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_body = then_body;
    node->data.if_stmt.then_count = then_count;
    node->data.if_stmt.else_body = else_body;
    node->data.if_stmt.else_count = else_count;
    return node;
}

// Parse for loop
static ASTNode *parse_for(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'for'
    
    Token *var = consume(parser, TOKEN_IDENTIFIER, "Expected loop variable");
    consume(parser, TOKEN_LPAREN, "Expected '(' after loop variable");
    
    ASTNode *start = parse_expression(parser);
    consume(parser, TOKEN_ELLIPSIS, "Expected '...' in for loop range");
    ASTNode *end = parse_expression(parser);
    
    // Optional step
    ASTNode *step = NULL;
    if (match(parser, TOKEN_COMMA)) {
        advance_parser(parser);
        step = parse_expression(parser);
    }
    
    consume(parser, TOKEN_RPAREN, "Expected ')' after loop range");
    
    // Optional label: for i (1...10) _myloop
    char *label = NULL;
    if (match(parser, TOKEN_UNDERSCORE)) {
        advance_parser(parser);
        Token *label_token = consume(parser, TOKEN_IDENTIFIER, "Expected label name after '_'");
        label = strdup(label_token->value);
    }
    
    // Parse body
    ASTNode **body = malloc(sizeof(ASTNode*) * 100);
    int body_count = 0;
    
    while (!match(parser, TOKEN_ENDL) && !match(parser, TOKEN_EOF)) {
        body[body_count++] = parse_statement(parser);
    }
    
    consume(parser, TOKEN_ENDL, "Expected 'endl' to close for loop");
    
    ASTNode *node = create_ast_node(AST_FOR_LOOP, token->line, token->column);
    node->data.for_loop.variable = strdup(var->value);
    node->data.for_loop.start = start;
    node->data.for_loop.end = end;
    node->data.for_loop.step = step;
    node->data.for_loop.body = body;
    node->data.for_loop.body_count = body_count;
    node->data.for_loop.label = label;
    return node;
}

// Parse while loop
static ASTNode *parse_while(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'while'
    
    // Optional parentheses
    int has_parens = 0;
    if (match(parser, TOKEN_LPAREN)) {
        has_parens = 1;
        advance_parser(parser);
    }
    
    ASTNode *condition = parse_expression(parser);
    
    if (has_parens) {
        consume(parser, TOKEN_RPAREN, "Expected ')' after condition");
    }
    
    // Optional label
    char *label = NULL;
    if (match(parser, TOKEN_UNDERSCORE)) {
        advance_parser(parser);
        Token *label_token = consume(parser, TOKEN_IDENTIFIER, "Expected label name after '_'");
        label = strdup(label_token->value);
    }
    
    // Parse body
    ASTNode **body = malloc(sizeof(ASTNode*) * 100);
    int body_count = 0;
    
    while (!match(parser, TOKEN_ENDL) && !match(parser, TOKEN_EOF)) {
        body[body_count++] = parse_statement(parser);
    }
    
    consume(parser, TOKEN_ENDL, "Expected 'endl' to close while loop");
    
    ASTNode *node = create_ast_node(AST_WHILE_LOOP, token->line, token->column);
    node->data.while_loop.condition = condition;
    node->data.while_loop.body = body;
    node->data.while_loop.body_count = body_count;
    node->data.while_loop.label = label;
    return node;
}

// Parse break/continue
static ASTNode *parse_break_continue(Parser *parser) {
    Token *token = current_token(parser);
    TokenType type = token->type;
    advance_parser(parser);
    
    // Optional label: break outer
    char *label = NULL;
    if (match(parser, TOKEN_IDENTIFIER)) {
        label = strdup(current_token(parser)->value);
        advance_parser(parser);
    }
    
    ASTNode *node = create_ast_node(type == TOKEN_BREAK ? AST_BREAK : AST_CONTINUE, 
                                    token->line, token->column);
    node->data.break_continue.label = label;
    return node;
}

// Parse halt
static ASTNode *parse_halt(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'halt'
    
    ASTNode *node = create_ast_node(AST_HALT, token->line, token->column);
    
    // Optional message
    if (match(parser, TOKEN_STRING)) {
        node->data.halt.message = parse_primary(parser);
    } else {
        node->data.halt.message = NULL;
    }
    
    return node;
}

// Parse inc/dec
static ASTNode *parse_inc_dec(Parser *parser) {
    Token *token = current_token(parser);
    TokenType op = token->type;
    advance_parser(parser);
    
    Token *var = consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    
    // Optional amount: inc x,5
    ASTNode *amount = NULL;
    if (match(parser, TOKEN_COMMA)) {
        advance_parser(parser);
        amount = parse_expression(parser);
    }
    
    ASTNode *node = create_ast_node(AST_UNARY_OP, token->line, token->column);
    node->data.unary_op.op = op;
    node->data.unary_op.variable = strdup(var->value);
    node->data.unary_op.amount = amount;
    return node;
}
// Parse function call: call .func(a,b) eq result
static ASTNode *parse_function_call(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'call'
    
    Token *func_name = consume(parser, TOKEN_LABEL, "Expected function name after 'call'");
    
    // Parse arguments
    consume(parser, TOKEN_LPAREN, "Expected '(' after function name");
    
    ASTNode **arguments = malloc(sizeof(ASTNode*) * 50);
    int arg_count = 0;
    
    while (!match(parser, TOKEN_RPAREN) && !match(parser, TOKEN_EOF)) {
        arguments[arg_count++] = parse_expression(parser);
        if (match(parser, TOKEN_COMMA)) {
            advance_parser(parser);
        }
    }
    
    consume(parser, TOKEN_RPAREN, "Expected ')' after arguments");
    
    // Optional result variables: call .func() eq x,y,z
    char **result_vars = NULL;
    int result_count = 0;
    
    if (match(parser, TOKEN_EQ)) {
        advance_parser(parser);
        result_vars = malloc(sizeof(char*) * 50);
        
        do {
            Token *var = consume(parser, TOKEN_IDENTIFIER, "Expected variable name after 'eq'");
            result_vars[result_count++] = strdup(var->value);
            
            if (match(parser, TOKEN_COMMA)) {
                advance_parser(parser);
            } else {
                break;
            }
        } while (1);
    }
    
    ASTNode *node = create_ast_node(AST_FUNCTION_CALL, token->line, token->column);
    node->data.function_call.function_name = strdup(func_name->value);
    node->data.function_call.arguments = arguments;
    node->data.function_call.arg_count = arg_count;
    node->data.function_call.result_vars = result_vars;
    node->data.function_call.result_count = result_count;
    return node;
}

// Parse return statement: ret x,y,z
static ASTNode *parse_return(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'ret'
    
    ASTNode **values = malloc(sizeof(ASTNode*) * 50);
    int value_count = 0;
    
    // Parse return values
    while (!match(parser, TOKEN_NEWLINE) && !match(parser, TOKEN_EOF)) {
        values[value_count++] = parse_expression(parser);
        
        if (match(parser, TOKEN_COMMA)) {
            advance_parser(parser);
        } else {
            break;
        }
    }
    
    ASTNode *node = create_ast_node(AST_RETURN, token->line, token->column);
    node->data.return_stmt.values = values;
    node->data.return_stmt.value_count = value_count;
    return node;
}

// Parse jump: jmp .label, jeq x,y .label
static ASTNode *parse_jump(Parser *parser) {
    Token *token = current_token(parser);
    TokenType jump_type = token->type;
    advance_parser(parser);
    
    ASTNode *left = NULL;
    ASTNode *right = NULL;
    
    // Conditional jumps: jeq x,y .label
    if (jump_type != TOKEN_JMP) {
        left = parse_expression(parser);
        consume(parser, TOKEN_COMMA, "Expected ',' after first operand");
        right = parse_expression(parser);
    }
    
    Token *label = consume(parser, TOKEN_LABEL, "Expected label for jump");
    
    ASTNode *node = create_ast_node(AST_JUMP, token->line, token->column);
    node->data.jump.jump_type = jump_type;
    node->data.jump.left = left;
    node->data.jump.right = right;
    node->data.jump.target_label = strdup(label->value);
    return node;
}

// Parse type check: type x or type t eq x
static ASTNode *parse_type_check(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser); // skip 'type'
    
    char *result_var = NULL;
    char *variable = NULL;
    
    // Check format: type t eq x or type x
    Token *first = consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    
    if (match(parser, TOKEN_EQ)) {
        // type t eq x
        result_var = strdup(first->value);
        advance_parser(parser);
        Token *var = consume(parser, TOKEN_IDENTIFIER, "Expected variable name after 'eq'");
        variable = strdup(var->value);
    } else {
        // type x
        variable = strdup(first->value);
        result_var = NULL;
    }
    
    ASTNode *node = create_ast_node(AST_TYPE_CHECK, token->line, token->column);
    node->data.type_check.variable = variable;
    node->data.type_check.result_var = result_var;
    return node;
}

// Parse function definition
static ASTNode *parse_function(Parser *parser) {
    Token *token = current_token(parser);
    
    Token *func_name = consume(parser, TOKEN_LABEL, "Expected function name");
    
    // Parse parameters
    consume(parser, TOKEN_LPAREN, "Expected '(' after function name");
    
    char **parameters = malloc(sizeof(char*) * 50);
    int param_count = 0;
    
    while (!match(parser, TOKEN_RPAREN) && !match(parser, TOKEN_EOF)) {
        Token *param = consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
        parameters[param_count++] = strdup(param->value);
        
        if (match(parser, TOKEN_COMMA)) {
            advance_parser(parser);
        }
    }
    
    consume(parser, TOKEN_RPAREN, "Expected ')' after parameters");
    
    // Parse function body (until we hit another function, start, or EOF)
    ASTNode **body = malloc(sizeof(ASTNode*) * 200);
    int body_count = 0;
    
    while (!match(parser, TOKEN_LABEL) && !match(parser, TOKEN_START) && !match(parser, TOKEN_EOF)) {
        body[body_count++] = parse_statement(parser);
    }
    
    ASTNode *node = create_ast_node(AST_FUNCTION, token->line, token->column);
    node->data.function.name = strdup(func_name->value);
    node->data.function.parameters = parameters;
    node->data.function.param_count = param_count;
    node->data.function.body = body;
    node->data.function.body_count = body_count;
    return node;
}

// Parse label definition
static ASTNode *parse_label_def(Parser *parser) {
    Token *token = current_token(parser);
    advance_parser(parser);
    
    ASTNode *node = create_ast_node(AST_LABEL, token->line, token->column);
    node->data.label.name = strdup(token->value);
    return node;
}

// Parse a single statement
static ASTNode *parse_statement(Parser *parser) {
    // Skip ALL newlines at start
    while (match(parser, TOKEN_NEWLINE)) {
        advance_parser(parser);
    }
    
    // Check for EOF
    if (match(parser, TOKEN_EOF)) {
        return NULL;
    }
    
    Token *token = current_token(parser);
    
    // Assignment
    if (match(parser, TOKEN_SET)) {
        return parse_assignment(parser);
    }
    
    // Echo
    if (match(parser, TOKEN_ECHO)) {
        return parse_echo(parser);
    }
    
    // Control flow
    if (match(parser, TOKEN_IF)) {
        return parse_if(parser);
    }
    
    if (match(parser, TOKEN_FOR)) {
        return parse_for(parser);
    }
    
    if (match(parser, TOKEN_WHILE)) {
        return parse_while(parser);
    }
    
    if (match(parser, TOKEN_BREAK) || match(parser, TOKEN_CONTINUE)) {
        return parse_break_continue(parser);
    }
    
    // Inc/Dec
    if (match(parser, TOKEN_INC) || match(parser, TOKEN_DEC)) {
        return parse_inc_dec(parser);
    }
    
    // Function call
    if (match(parser, TOKEN_CALL)) {
        return parse_function_call(parser);
    }
    
    // Return
    if (match(parser, TOKEN_RET)) {
        return parse_return(parser);
    }
    
    // Jumps
    if (match(parser, TOKEN_JMP) || match(parser, TOKEN_JEQ) || 
        match(parser, TOKEN_JNE) || match(parser, TOKEN_JGT) || 
        match(parser, TOKEN_JLT) || match(parser, TOKEN_JGE) || 
        match(parser, TOKEN_JLE)) {
        return parse_jump(parser);
    }
    
    // Halt
    if (match(parser, TOKEN_HALT)) {
        return parse_halt(parser);
    }
    
    // Type check
    if (match(parser, TOKEN_TYPE)) {
        return parse_type_check(parser);
    }
    
    // Label definition (inside main)
    if (match(parser, TOKEN_LABEL)) {
        return parse_label_def(parser);
    }
    
    // Operations without assignment (just for side effects in conditions)
    if (match(parser, TOKEN_ADD) || match(parser, TOKEN_SUB) || 
        match(parser, TOKEN_MUL) || match(parser, TOKEN_DIV) || 
        match(parser, TOKEN_MOD)) {
        return parse_expression(parser);
    }
    
    fprintf(stderr, "Parse Error [%d:%d]: Unexpected token %s in statement\n",
            token->line, token->column, token_type_name(token->type));
    exit(1);
}

// Main parse function
ASTNode *parse(Token **tokens, int token_count) {
    Parser *parser = create_parser(tokens, token_count);
    
    ASTNode *program = create_ast_node(AST_PROGRAM, 1, 0);
    ASTNode **statements = malloc(sizeof(ASTNode*) * 1000);
    int statement_count = 0;
    
    // Skip initial newlines
    while (match(parser, TOKEN_NEWLINE)) {
        advance_parser(parser);
    }
    
    // Parse the program
    while (!match(parser, TOKEN_EOF)) {
        Token *token = current_token(parser);
        
        // Function definition: .funcName(params)
        if (match(parser, TOKEN_LABEL) && peek_token(parser, 1)->type == TOKEN_LPAREN) {
            statements[statement_count++] = parse_function(parser);
        }
        // Start main
        else if (match(parser, TOKEN_START)) {
            advance_parser(parser); // skip 'start'
            Token *main_label = consume(parser, TOKEN_LABEL, "Expected .main after 'start'");
            
            if (strcmp(main_label->value, ".main") != 0) {
                fprintf(stderr, "Parse Error: Expected '.main' after 'start'\n");
                exit(1);
            }
// Parse main body (until EOF or next function)
        while (!match(parser, TOKEN_EOF) && 
            !(match(parser, TOKEN_LABEL) && peek_token(parser, 1)->type == TOKEN_LPAREN)) {
            ASTNode *stmt = parse_statement(parser);
            if (stmt != NULL) {
                 statements[statement_count++] = stmt;
            }
        }
        }
        else {
            advance_parser(parser);
        }
    }
    
    program->data.program.statements = statements;
    program->data.program.statement_count = statement_count;
    
    free_parser(parser);
    return program;
}

// Free AST node (recursive)
void free_ast_node(ASTNode *node) {
    if (!node) return;
    
    // TODO: Free all nested nodes and allocated strings
    // This is complex, implement as needed
    
    free(node);
}

// Print AST (for debugging)
void print_ast(ASTNode *node, int indent) {
    if (!node) {
        for (int i = 0; i < indent; i++) printf("  ");
        printf("(NULL NODE)\n");
        return;
    }
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("PROGRAM (%d statements)\n", node->data.program.statement_count);
            for (int i = 0; i < node->data.program.statement_count; i++) {
                printf("  Statement %d:\n", i);
                if (node->data.program.statements[i] == NULL) {
                    printf("    (NULL)\n");
                } else {
                    print_ast(node->data.program.statements[i], indent + 1);
                }
            }
            break;
            
        case AST_ASSIGNMENT:
            printf("ASSIGNMENT: %s = ", node->data.assignment.variable ? node->data.assignment.variable : "(null var)");
            if (node->data.assignment.value) {
                printf("\n");
                print_ast(node->data.assignment.value, indent + 1);
            } else {
                printf("(null value)\n");
            }
            break;
            
        case AST_ECHO:
            printf("ECHO (%d expressions)\n", node->data.echo.expr_count);
            for (int i = 0; i < node->data.echo.expr_count; i++) {
                if (node->data.echo.expressions[i]) {
                    print_ast(node->data.echo.expressions[i], indent + 1);
                } else {
                    for (int j = 0; j < indent + 1; j++) printf("  ");
                    printf("(NULL expression)\n");
                }
            }
            break;
            
        case AST_LITERAL_INT:
            printf("INT: %d\n", node->data.int_literal.value);
            break;
            
        case AST_LITERAL_FLOAT:
            printf("FLOAT: %f\n", node->data.float_literal.value);
            break;
            
        case AST_LITERAL_STRING:
            printf("STRING: \"%s\"\n", node->data.string_literal.value ? node->data.string_literal.value : "(null)");
            break;
            
        case AST_LITERAL_BOOL:
            printf("BOOL: %s\n", node->data.bool_literal.value ? "true" : "false");
            break;
            
        case AST_IDENTIFIER:
            printf("IDENTIFIER: %s\n", node->data.identifier.name ? node->data.identifier.name : "(null)");
            break;
            
        case AST_BINARY_OP:
            printf("BINARY_OP: %s", token_type_name(node->data.binary_op.op));
            if (node->data.binary_op.result) {
                printf(" -> %s", node->data.binary_op.result);
            }
            printf("\n");
            if (node->data.binary_op.left) {
                print_ast(node->data.binary_op.left, indent + 1);
            } else {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("(NULL left)\n");
            }
            if (node->data.binary_op.right) {
                print_ast(node->data.binary_op.right, indent + 1);
            } else {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("(NULL right)\n");
            }
            break;
            
        case AST_ARRAY:
            printf("ARRAY (%d elements)\n", node->data.array.element_count);
            break;
            
        case AST_INPUT:
            printf("INPUT\n");
            break;
            
        case AST_FOR_LOOP:
            printf("FOR_LOOP: %s\n", node->data.for_loop.variable ? node->data.for_loop.variable : "(null)");
            break;
            
        case AST_IF_STATEMENT:
            printf("IF_STATEMENT\n");
            break;
            
        case AST_LABEL:
            printf("LABEL: %s\n", node->data.label.name ? node->data.label.name : "(null)");
            break;
            
        default:
            printf("NODE_TYPE: %d (unknown)\n", node->type);
            break;
    }
}
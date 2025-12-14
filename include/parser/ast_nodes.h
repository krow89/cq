#ifndef AST_NODES_H
#define AST_NODES_H

#include "parser.h"

/* AST node memory management */
void retainNode(ASTNode* node);
void releaseNode(ASTNode* node);

/* node creation helpers */
ASTNode* create_node(ASTNodeType type);
ASTNode* create_identifier_node(const char* name);
ASTNode* create_literal_node(const char* value);

/* debugging */
void printAst(ASTNode* node, int depth);

#endif /* AST_NODES_H */

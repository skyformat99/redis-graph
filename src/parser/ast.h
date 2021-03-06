/*
* Copyright 2018-2019 Redis Labs Ltd. and Contributors
*
* This file is available under the Apache License, Version 2.0,
* modified with the Commons Clause restriction.
*/

#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include "../value.h"
#include "./ast_common.h"
#include "../redismodule.h"
#include "../rmutil/vector.h"
#include "./clauses/clauses.h"

typedef enum {
	AST_VALID,
	AST_INVALID
} AST_Validation;

typedef struct {
	AST_MatchNode *matchNode;
	AST_CreateNode *createNode;
	AST_MergeNode *mergeNode;
	AST_SetNode *setNode;
	AST_DeleteNode *deleteNode;
	AST_WhereNode *whereNode;
	AST_ReturnNode *returnNode;
	AST_OrderNode *orderNode;
	AST_LimitNode *limitNode;
	AST_IndexNode *indexNode;
} AST_Query;

AST_Query* New_AST_Query(AST_MatchNode *matchNode, AST_WhereNode *whereNode,
						 AST_CreateNode *createNode, AST_MergeNode *mergeNode,
						 AST_SetNode *setNode, AST_DeleteNode *deleteNode,
						 AST_ReturnNode *returnNode, AST_OrderNode *orderNode,
						 AST_LimitNode *limitNode, AST_IndexNode *indexNode);

// AST clause validations.
AST_Validation AST_Validate(const AST_Query* ast, char **reason);

void AST_NameAnonymousNodes(AST_Query *ast);

// Checks if AST represent a read only query.
bool AST_ReadOnly(const AST_Query *ast);

void Free_AST_Query(AST_Query *queryExpressionNode);

#endif

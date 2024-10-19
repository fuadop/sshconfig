#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdbool.h>

/* LEXER APIs */

#define KEYWORDS_COUNT 7

typedef enum {
	KEYWORD,
	IDENTIFIER,
	COMMENT,
	END,
} TokenType;

typedef struct {
	char* value;
	TokenType type;
} Token;

Token next(FILE *f);

/* PARSER APIs */

typedef struct {
	char* key;
	char* value;
} Attribute;

typedef struct {
	int len;
	Attribute *attributes;
} Rule;

typedef struct {
	int len;
	Rule *rules;
} Config;

Config parse(FILE *f);
void free_config(Config c);

/* UTIL APIs */
typedef struct {
	int len;
	char **list;
} StringList;

StringList expand_path(char *str);

#endif

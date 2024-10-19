#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include "parser.h"

const char *KEYWORDS[KEYWORDS_COUNT] = {
	"User",
	"Host",
	"Include",
	"HostName",
	"IdentityFile",
	"IdentityAgent",
	"IdentitiesOnly",
};

bool is_whitespace(char c) {
	return c == ' ' ||
		c == '\n' ||
		c == '\t' ||
		c == '\r';
}

void skip_whitespace(FILE *f) {
	if(feof(f)) return;

	char c;
	fread(&c, sizeof(char), 1, f);

	while (is_whitespace(c)) {
		fread(&c, sizeof(char), 1, f);

		if(feof(f)) return;
	}

	fseek(f, -1, SEEK_CUR);
}

bool is_comment(char *str) {
	return str[0] == '#';
}

bool is_keyword(char *str) {
	for (int i = 0; i < KEYWORDS_COUNT; i++) {
		if (strcmp(KEYWORDS[i], str) == 0) {
			return true;
		}
	}

	return false;
}

Token next(FILE *f) {
	skip_whitespace(f);

	char c;
	fread(&c, sizeof(char), 1, f);

	int len = 1;
	Token t = {
		.type = END,
		.value = NULL,
	}; 

	if (feof(f)) return t;

	switch (c) {
		case '#':
			{
				while (c != '\n') {
					fread(&c, sizeof(char), 1, f);
					len++;

					if(feof(f)) break;
				}
			}
			break;
		case '"':
			{
				fread(&c, sizeof(char), 1, f);
				len++;

				while (c != '"') {
					fread(&c, sizeof(char), 1, f);
					len++;

					if(feof(f)) break;
				}

				fread(&c, sizeof(char), 1, f);
				len++;
			}
			break;
		default:
			{
				while (is_whitespace(c) == false) {
					fread(&c, sizeof(char), 1, f);
					len++;

					if(feof(f)) break;
				}
			}
	}

	t.value = malloc(sizeof(char) * len);
	memset(t.value, 0, sizeof(char) * len);

	fseek(f, -len, SEEK_CUR);
	fread(t.value, sizeof(char) * (len - 1), 1, f);

	t.type = is_keyword(t.value) ? KEYWORD : 
		is_comment(t.value) ? COMMENT :
		IDENTIFIER;

	return t;
}

char* to_string(TokenType t) {
	if (t == END) return "END";
	if (t == KEYWORD) return "KEYWORD";
	if (t == COMMENT) return "COMMENT";
	if (t == IDENTIFIER) return "IDENTIFIER";

	return "UNKNOWN";
}

StringList expand_path(char *str) {
	StringList l = {
		.len = 0,
		.list = NULL,
	};

	if (str[0] == '~') {
		char *home = getenv("HOME");
		char *newstr = malloc(sizeof(char) * (strlen(str) + strlen(home)));

		memset(newstr, 0, sizeof(char) * (strlen(str) + strlen(home)));

		memcpy(newstr, home, strlen(home));
		memcpy(newstr+strlen(home), str+1, strlen(str) - 1);

		str = newstr;
	}

	if (str[strlen(str) - 1] == '*') {
		str[strlen(str) - 1] = 0;

		DIR *dir = opendir(str);
		if (dir == NULL) {
			printf("unable to read directory: %s\n", str);
			exit(1);
		}

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			bool is_meta = strcmp(entry->d_name, ".") == 0 ||strcmp(entry->d_name, "..") == 0;

			if (is_meta == false) {
				char *fullpath = malloc(sizeof(char) * (strlen(str) + entry->d_namlen + 1));
				memset(fullpath, 0, sizeof(char) * (strlen(str) + entry->d_namlen + 1));

				memcpy(fullpath, str, strlen(str));
				memcpy(fullpath+strlen(str), entry->d_name, entry->d_namlen);

				l.len++;
				l.list= realloc(l.list, sizeof(char*) * l.len);

				l.list[l.len - 1] = fullpath;
			}
		}

		closedir(dir);
	} else {
		l.len++;
		l.list= realloc(l.list, sizeof(char*) * l.len);

		l.list[l.len - 1] = str;
	}

	return l;
}

void free_config(Config c) {
	if (c.rules != NULL) {
		for (int i = 0; i < c.len; i++) {
			Rule *r = &c.rules[i];

			if (r->attributes != NULL) {
				for (int j = 0; j < r->len; j++) {
					Attribute *a = &r->attributes[j];

					if (a->key != NULL) free(a->key);
					if (a->value != NULL) free(a->value);
				}

				free(r->attributes);
			}
		}

		free(c.rules);
	}
}

Config parse(FILE *f) {
	fseek(f, 0, SEEK_SET);
	Config c = { .len = 0, .rules = NULL };

	Token t;
	while (t.type != END) {
		t = next(f);

		if (t.type == END) break;
		if (t.type == COMMENT) continue;

		if (t.type == KEYWORD) {
			Token v = next(f);

			if (v.type != IDENTIFIER) {
				printf("expected %s but got %s", to_string(IDENTIFIER), to_string(v.type));
				exit(1);
			}

			if (strcmp(t.value, "Include") == 0) {
				StringList paths = expand_path(v.value);

				for (int i = 0; i < paths.len; i++) {
					char *path = paths.list[i];

					FILE *nf = fopen(path, "rb");
					Config nc = parse(nf);

					for (int i = 0; i < nc.len; i++) {
						c.len++;
						c.rules = realloc(c.rules, sizeof(Rule) * c.len);

						c.rules[c.len - 1] = nc.rules[i];
					}

					free(path);
					fclose(nf);
					free(nc.rules);
				}

				continue;
			}


			if (strcmp(t.value, "Host") == 0) {
				c.len++;
				c.rules = realloc(c.rules, sizeof(Rule) * c.len);
			}

			Rule *r = &c.rules[c.len-1];

			r->len++;
			r->attributes = realloc(r->attributes, sizeof(Attribute) * r->len);

			Attribute a = {
				.key = t.value,
				.value = v.value,
			};

			r->attributes[r->len-1] = a;

			continue;
		}

		printf("expected %s but got %s", to_string(KEYWORD), to_string(t.type));
		exit(1);
	}

	return c;
}


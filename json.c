#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./json.h"
#define BLOCK 1024
bool contains_char(char c, char *chars) {
	if (c < 0 || c > 127) return 0;
	for (; *chars; ++chars) {
		if (*chars == c) return 1;
	}
	return 0;
}
bool is_syntax_or_whitespace(char c) {
	if (c < 0) return 1;
	if (c > 127) return 0;
	if (contains_char(c, syntax_chars)) return 1;
	if (contains_char(c, whitespace_chars)) return 1;
	return 0;
}
enum json_value_type get_type(FILE *fp) {
#define G fgetc(fp)
	int c = G;
	if (c < 0 || c > 127) {
		return NONE;
	} else if (c == 't') {
		if (G != 'r') return NONE;
		if (G != 'u') return NONE;
		if (G != 'e') return NONE;
		int c2 = G;
		if (c2 >= 0) ungetc(c2, fp);
		if (!is_syntax_or_whitespace(c2)) return NONE;
		return TRUE;
	} else if (c == 'f') {
		if (G != 'a') return NONE;
		if (G != 'l') return NONE;
		if (G != 's') return NONE;
		if (G != 'e') return NONE;
		int c2 = G;
		if (c2 >= 0) ungetc(c2, fp);
		if (!is_syntax_or_whitespace(c2)) return NONE;
		return FALSE;
	} else if (c == 'n') {
		if (G != 'u') return NONE;
		if (G != 'l') return NONE;
		if (G != 'l') return NONE;
		int c2 = G;
		if (c2 >= 0) ungetc(c2, fp);
		if (!is_syntax_or_whitespace(c2)) return NONE;
		return NULL_;
	} else if (c == 'u') {
		if (G != 'n') return NONE;
		if (G != 'd') return NONE;
		if (G != 'e') return NONE;
		if (G != 'f') return NONE;
		if (G != 'i') return NONE;
		if (G != 'n') return NONE;
		if (G != 'e') return NONE;
		if (G != 'e') return NONE;
		int c2 = G;
		if (c2 >= 0) ungetc(c2, fp);
		if (!is_syntax_or_whitespace(c2)) return NONE;
		return UNDEFINED;
	} else if (c == 'I') {
		if (G != 'n') return NONE;
		if (G != 'f') return NONE;
		if (G != 'i') return NONE;
		if (G != 'n') return NONE;
		if (G != 'i') return NONE;
		if (G != 't') return NONE;
		if (G != 'y') return NONE;
		int c2 = G;
		if (c2 >= 0) ungetc(c2, fp);
		if (!is_syntax_or_whitespace(c2)) return NONE;
		return INFINITY;
	} else if (c == 'N') {
		if (G != 'a') return NONE;
		if (G != 'N') return NONE;
		int c2 = G;
		if (c2 >= 0) ungetc(c2, fp);
		if (!is_syntax_or_whitespace(c2)) return NONE;
		return NAN;
	} else if (c == '"') {
		return STRING;
	} else if (c == '[') {
		return ARRAY;
	} else if (c == '{') {
		return OBJECT;
	} else if (contains_char(c, number_chars)) {
		ungetc(c, fp);
		return NUMBER;
	}
	return NONE;
}
char *parse_string(FILE *fp) {
	char *in = NULL;
	size_t in_len = 0;
	size_t in_real_len = 0;
	for (;;) {
		char c = fgetc(fp);
		if (c < 0) return NULL;
		if (c == '"') {
			if (!in) in = malloc(1);
			if (in) in[in_len] = '\0';
			return in;
		}
		++in_len;
		if (in_len >= in_real_len) {
			in_real_len += BLOCK;
			in = realloc(in, in_real_len + 1);
			if (!in) return NULL;
		}
		in[in_len-1] = c;
	}
	return NULL;
}
struct json_number parse_number(FILE *fp) {
	struct json_number number;
	number.type = NONE;
	for (;;) {
		char c = fgetc(fp);
		if (c < 0) return number;
		if (contains_char(c, number_chars)) {
		} else if (is_syntax_or_whitespace(c)) {
			ungetc(c, fp);
			number.long_ = 1;
			number.type = NUMBER_LONG;
			return number;
		} else return number;
	}
	return number;
}
struct json_value parse_value(FILE *fp, enum json_value_type type) {
	struct json_value j;
	j.type = type;
	switch (type) {
		case STRING:
			j.string = parse_string(fp);
			break;
		case ARRAY:
			j.array = NULL;
			break;
		case OBJECT:
			j.object = NULL;
			break;
		case NUMBER:
			j.number = parse_number(fp);
			break;
		default:
			break;
	}
	return j;
}
struct json_value parse_json(FILE *fp) {
	enum json_value_type type = NONE;
	type = get_type(fp);
	struct json_value v;
	v.type = NONE;
	if (type != NONE) v = parse_value(fp, type);
	return v;
}
bool print_json(struct json_value v, int indent) {
#define INDENT() { for (int i = 0; i < indent; ++i) { fputc('\t', stdout); } }
	INDENT();
	switch (v.type) {
		case NONE:
			printf("error\n");
			return 0;
		case NULL_:
			printf("null\n");
			return 1;
		case UNDEFINED:
			printf("undefined\n");
			return 1;
		case TRUE:
			printf("true\n");
			return 1;
		case FALSE:
			printf("false\n");
			return 1;
		case NUMBER:
			switch (v.number.type) {
				case NUMBER_LONG:
					printf("%li\n", v.number.long_);
					return 1;
				case NUMBER_DOUBLE:
					printf("%lf\n", v.number.double_);
					return 1;
				default:
					printf("error\n");
					return 0;
			}
		case NAN:
			printf("NaN\n");
			return 1;
		case INFINITY:
			printf("Infinity\n");
			return 1;
		case ARRAY:
			printf("[\n");
			printf("]\n");
			return 1;
		case OBJECT:
			printf("{\n");
			printf("}\n");
			return 1;
		case STRING:
			printf("\"%s\"\n", v.string);
			return 1;
	}
	return 0;
}
int main() {
	struct json_value v = parse_json(stdin);
	print_json(v, 0);
	return 0;
}

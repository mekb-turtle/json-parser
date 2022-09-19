#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./json.h"
#define BLOCK 1024
char *syntax_chars     = ",{}[]";
char *whitespace_chars = " \t\n\r\v";
char *number_chars     = "0123456789.-e";
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
		int c = fgetc(fp);
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
	number.type = NUMBER_NONE;
	for (;;) {
		int c = fgetc(fp);
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
	if (type == NONE) return j;
	switch (type) {
		case STRING:
			j.string = parse_string(fp);
			if (!j.string) j.type = NONE;
			break;
		case ARRAY:
			struct json_array *array = malloc(sizeof(struct json_array));
			if (!array) break;
			struct json_value *values = NULL;
			size_t array_len = 0;
			size_t array_real_len = 0;
			bool needs_comma = 0;
			for (;;) {
				int c = fgetc(fp);
				if (c < 0) { j.type = NONE; break; }
				if (c == ']') {
					if (values == NULL) {
						values = malloc(sizeof(struct json_value));
						if (values) values[0].type = NONE;
					}
					break;
				}
				if (c == '}') {
					free(values); free(array);
					j.type = NONE; break;
				} else if (needs_comma != (c == ',')) {
					free(values); free(array);
					j.type = NONE; break;
				} else if (c == ',' && needs_comma) {
					needs_comma = 0;
				} else if (!contains_char(c, whitespace_chars)) {
					ungetc(c, fp);
					struct json_value v = parse_value(fp, get_type(fp));
					if (v.type == NONE) {
						free(values); free(array);
						j.type = NONE; break;
					}
					++array_len;
					if (array_len >= array_real_len) {
						array_real_len += BLOCK;
						values = realloc(values, sizeof(struct json_value) * (array_real_len + 1));
						if (!values) {
							free(values); free(array);
							j.type = NONE; break;
						}
					}
					values[array_len-1] = v;
					needs_comma = 1;
				}
			}
			if (j.type == NONE || j.type == NONE) break;
			values[array_len].type = NONE;
			array->values = values;
			j.array = array;
			break;
		case OBJECT:
			j.object = NULL;
			break;
		case NUMBER:
			j.number = parse_number(fp);
			if (j.number.type == NUMBER_NONE) j.type = NONE;
			break;
		default:
			break;
	}
	return j;
}
struct json_value parse_json(FILE *fp) {
	for (;;) {
		int c = fgetc(fp);
		if (c < 0) {
			struct json_value v;
			v.type = NONE;
			return v;
		}
		if (!contains_char(c, whitespace_chars)) {
			ungetc(c, fp);
			break;
		}
	}
	return parse_value(fp, get_type(fp));
}
bool print_json(struct json_value v, int indent) {
#define INDENT() { for (int i = 0; i < indent; ++i) { fputc('\t', stdout); } }
#define PUTS(x) fputs(x, stdout)
#define PUTC(x) fputc(x, stdout)
	INDENT();
	switch (v.type) {
		case NULL_:
			PUTS("null");
			return 1;
		case UNDEFINED:
			PUTS("undefined");
			return 1;
		case TRUE:
			PUTS("true");
			return 1;
		case FALSE:
			PUTS("false");
			return 1;
		case NUMBER:
			switch (v.number.type) {
				case NUMBER_LONG:
					printf("%li", v.number.long_);
					return 1;
				case NUMBER_DOUBLE:
					printf("%lf", v.number.double_);
					return 1;
				default:
					PUTS("error");
					return 0;
			}
		case NAN:
			PUTS("NaN");
			return 1;
		case INFINITY:
			PUTS("Infinity");
			return 1;
		case ARRAY:
			if (v.array->values->type != NONE) {
				PUTS("[\n");
				for (int u=0; v.array->values->type != NONE; ++v.array->values,++u) {
					print_json(*v.array->values, indent+1);
					if ((v.array->values+1)->type != NONE) PUTC(',');
					PUTC('\n');
				}
				PUTC(']');
			} else {
				PUTS("[]");
			}
			return 1;
		case OBJECT:
			PUTC('{');
			PUTC('}');
			return 1;
		case STRING:
			printf("\"%s\"", v.string);
			return 1;
		case NONE:
			PUTS("error");
			return 0;
		default:
			PUTS("unknown type");
			return 0;
	}
	return 0;
}

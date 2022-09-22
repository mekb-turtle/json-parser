#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "./json.h"
#define BLOCK 1024
#define MAX_NUMBER_SIZE 16
char *syntax_chars            = ",{}[]";
char *whitespace_chars        = "\t\n\r\v ";
char *string_disallowed_chars = "\0\n\r";
char *number_chars            = "0123456789.-+e";
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
	char *string = NULL;
	size_t string_len = 0;
	size_t string_real_len = 0;
	int a, a1, a2;
	for (;;) {
		int c = fgetc(fp);
		if (c < 0) return NULL;
		if (c == '"') {
			if (!string) string = malloc(1);
			if (string) string[string_len] = '\0';
			return string;
		} else if (c == '\\') {
			c = fgetc(fp);
			if (c < 0) return NULL;
			bool e = 0, d = 0;
			switch (c) {
				case '"':
				case '\\':
				case '/':
					break;
				case '0': c = '\x00'; break;
				case 'b': c = '\x08'; break;
				case 't': c = '\x09'; break;
				case 'n': c = '\x0a'; break;
				case 'v': c = '\x0b'; break;
				case 'f': c = '\x0c'; break;
				case 'r': c = '\x0d'; break;
				case 'e': c = '\x1b'; break;
#define ADD(c) { ++string_len;\
					if (string_len >= string_real_len) {\
						string_real_len += BLOCK;\
						string = realloc(string, string_real_len + 1);\
						if (!string) return NULL;\
					}\
					string[string_len-1] = c; }
#define C(c, d) { c = fgetc(fp);\
					if (c >= '0' && c <= '9') d = c - '0';\
					else if (c >= 'a' && c <= 'f') d = c - 'a' + 0xa;\
					else { e = 1; break; } }
#define D() { C(a, a1); C(a, a2); ADD(a1 << 4 | a2); }
				/*
				case 'U':
					D();
					D();
				case 'u':
					D();
				*/
				case 'x':
					D();
					d = 1;
					break;
				default: e = 1; break;
			}
			if (e) break;
			if (d) continue;
		} else if (contains_char(c, string_disallowed_chars)) break;
		ADD(c);
	}
	free(string); return NULL;
}
struct json_number parse_number(FILE *fp) {
	struct json_number number;
	number.type = NUMBER_NONE;
	bool exponent = 0;
	bool point = 0;
	bool negative, negativee, positive, positivee = 0;
	bool can_sign = 1;
	// n = number
	// np = number after decimal point
	// ne = second part of exponent
	// npe = second part of exponent after decimal point
	char *n = NULL, *np = NULL, *ne = NULL;
	size_t nl   = 0,
	       nlp  = 0,
	       nle  = 0;
	// TODO: make code less messy
#define END() { free(n); free(np); free(ne); return number; }
#define ADD_NUM(c, a, len) {\
		if (len >= MAX_NUMBER_SIZE) END();\
		++len;\
		if (!a) {\
			a = malloc(MAX_NUMBER_SIZE);\
			if (!a) END();\
		}\
		a[len-1] = c; }
	for (;;) {
		int c = fgetc(fp);
		if (c < 0) break;
		if (contains_char(c, number_chars)) {
			if (c >= '0' && c <= '9') {
				if (exponent) {
					ADD_NUM(c, ne,  nle);
				} else if (point) {
					ADD_NUM(c, np,  nlp);
				} else {
					ADD_NUM(c, n,   nl);
				}
				can_sign = 0;
			} else if (c == '-' || c == '+') {
				if (!can_sign) END();
				if (exponent) {
					if (negativee || positivee) END();
					if (c == '-') negativee = 1; else positivee = 1;
				} else {
					if (negative  || positive)  END();
					if (c == '-') negative  = 1; else positive  = 1;
				}
			} else if (c == 'e') {
				if (point && !np) END();
				if (exponent) END();
				can_sign = 1;
				exponent = 1;
				point = 0;
			} else if (c == '.') {
				if (exponent) {
					END();
				} else {
					if (point) END();
					if (!n) ADD_NUM('0', n,  nl);
					point = 1;
				}
			} else END();
		} else if (is_syntax_or_whitespace(c)) {
			ungetc(c, fp);
			break;
		} else {
			END();
		}
	}
	if (exponent && !ne) END();
	if (point && !np) END();
	size_t j;
	char *n_ = n, *ne_ = ne;
	if (n)   { for (j = 0; n [j] == '0'; ++j, ++n_,  --nl);  }
	if (ne)  { for (j = 0; ne[j] == '0'; ++j, ++ne_, --nle); }
	if (np)  { while (np [nlp -1] == '0') --nlp;  }
	if (nlp == 1 && np [0] == '0') { free(np); np  = NULL; nlp = 0; }
	if (n_)  n_  [nl]   = '\0';
	if (ne_) ne_ [nle]  = '\0';
	if (np)  np  [nlp]  = '\0';
	long int i = 0, ie = 0, ip = 0, ie2 = 0;
	if (n_)  i  = strtol(n_,  NULL, 10);
	if (ne_) ie = strtol(ne_, NULL, 10);
	if (np)  ip = strtol(np,  NULL, 10);
	bool can_be_int = 1;
	if (np && (!ne_ || nlp > ie)) can_be_int = 0;
	if (can_be_int) {
		for (ie2 = ie-nlp; ie2 > 0; --ie2) ip *= 10;
		for (; ie > 0; --ie) i *= 10;
		number.long_ = ip + i;
		number.type = NUMBER_LONG;
	} else {
		double f = i;
		double fp = ip;
		nlp -= ie;
		for (; nlp > 0; --nlp) fp /= 10;
		for (; ie  > 0; --ie)  f  *= 10;
		number.double_ = f + fp;
		number.type = NUMBER_DOUBLE;
	}
	END();
}
struct json_value parse_value(FILE *fp, enum json_value_type type) {
	struct json_value j;
	j.type = type;
	if (type == NONE) return j;
	bool needs_comma;
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
			needs_comma = 0;
			j.type = NONE;
#define ARRAY_BREAK() {\
				free(values); free(array);\
				values = NULL; array = NULL; break; }
			for (;;) {
				int c = fgetc(fp);
				if (c < 0) ARRAY_BREAK();
				if (contains_char(c, whitespace_chars)) {
					continue;
				} else if (c == ']') {
					if (!needs_comma && array_len > 0) ARRAY_BREAK();
					if (values == NULL) {
						values = malloc(sizeof(struct json_value));
						if (values) values[0].type = NONE;
					} else {
						values[array_len].type = NONE;
					}
					break;
				} else if (c == '}') {
					ARRAY_BREAK();
				} else if (needs_comma != (c == ',')) {
					ARRAY_BREAK();
				} else if (c == ',' && needs_comma) {
					needs_comma = 0;
				} else {
					ungetc(c, fp);
					struct json_value v = parse_value(fp, get_type(fp));
					if (v.type == NONE) ARRAY_BREAK();
					++array_len;
					if (array_len >= array_real_len) {
						array_real_len += BLOCK;
						values = realloc(values, sizeof(struct json_value) * (array_real_len + 1));
						if (!values) ARRAY_BREAK();
					}
					values[array_len-1] = v;
					needs_comma = 1;
				}
			}
			if (!array || !values) break;
			j.type = ARRAY;
			values[array_len].type = NONE;
			array->values = values;
			j.array = array;
			break;
		case OBJECT:
			struct json_object *object = malloc(sizeof(struct json_object));
			if (!object) break;
			struct json_object_entry *entries = NULL;
			size_t object_len = 0;
			size_t object_real_len = 0;
			needs_comma = 0;
			bool needs_colon = 0;
			bool is_value = 0;
			j.type = NONE;
#define OBJECT_BREAK() {\
				free(entries); free(object);\
				entries = NULL; object = NULL; break; }
			for (;;) {
				int c = fgetc(fp);
				if (c < 0) OBJECT_BREAK();
				if (contains_char(c, whitespace_chars)) {
					continue;
				} else if (c == '}') {
					if (needs_colon || is_value) OBJECT_BREAK();
					if (!needs_comma && object_len > 0) OBJECT_BREAK();
					if (entries == NULL) {
						entries = malloc(sizeof(struct json_object_entry));
						if (entries) entries[0].value.type = NONE;
					} else {
						entries[object_len].value.type = NONE;
					}
					break;
				} else if (c == ']') {
					OBJECT_BREAK();
				} else if (needs_colon != (c == ':')) { 
					OBJECT_BREAK();
				} else if (c == ':') {
					is_value = 1;
					needs_colon = 0;
				} else if (needs_comma != (c == ',')) {
					OBJECT_BREAK();
				} else if (c == ',' && needs_comma) {
					needs_comma = 0;
				} else if (!contains_char(c, whitespace_chars)) {
					ungetc(c, fp);
					enum json_value_type type = get_type(fp);
					struct json_value v;
					if (!is_value) {
						if (type != STRING) OBJECT_BREAK();
					}
					v = parse_value(fp, type);
					if (v.type == NONE) OBJECT_BREAK();
					if (is_value) {
						entries[object_len-1].value = v;
						needs_comma = 1;
						needs_colon = 0;
						is_value = 0;
					} else {
						++object_len;
						if (object_len >= object_real_len) {
							object_real_len += BLOCK;
							entries = realloc(entries, sizeof(struct json_object_entry) * (object_real_len + 1));
							if (!entries) OBJECT_BREAK();
						}
						entries[object_len-1].key = v.string;
						needs_colon = 1;
						printf("here\n");
					}
				}
			}
			if (!object || !entries) break;
			j.type = OBJECT;
			entries[object_len].value.type = NONE;
			object->entries = entries;
			j.object = object;
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
#define PUTS(x) fputs(x, stdout)
#define PUTC(x) fputc(x, stdout)
void print_string(char *str) {
	for (; *str; ++str) {
		switch (*str) {
			case '"':
			case '\\':
				printf("\\%c", *str); break;
			case '\x00':
				PUTS("\\0"); break;
			case '\x08':
				PUTS("\\b"); break;
			case '\x09':
				PUTS("\\t"); break;
			case '\x0a':
				PUTS("\\n"); break;
			case '\x0b':
				PUTS("\\v"); break;
			case '\x0c':
				PUTS("\\f"); break;
			case '\x0d':
				PUTS("\\r"); break;
			default:
				if (*str < '\x20' || (*str >= '\x7f' && *str <= '\xa0') || *str == '\xad')
					printf("\\%02x", *str);
				else
					PUTC(*str);
				break;
		}
	}
}
bool print_json(struct json_value v, int indent) {
#define INDENT() { for (int i = 0; i < indent; ++i) { fputc('\t', stdout); } }
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
			if (v.array->values[0].type != NONE) {
				PUTS("[\n");
				for (size_t i = 0; v.array->values[i].type != NONE; ++i) {
					print_json(v.array->values[i], indent+1);
					if (v.array->values[i+1].type != NONE) PUTC(',');
					PUTC('\n');
				}
				INDENT();
				PUTC(']');
			} else {
				PUTS("[]");
			}
			return 1;
		case OBJECT:
			if (v.object->entries[0].value.type != NONE) {
				PUTS("{\n");
				for (size_t i = 0; v.object->entries[i].value.type != NONE; ++i) {
					print_json(v.object->entries[i].value, indent+1);
					print_json(v.object->entries[i].value, indent+1);
					if (v.object->entries[i+1].value.type != NONE) PUTC(',');
					PUTC('\n');
				}
				INDENT();
				PUTC('}');
			} else {
				PUTS("{}");
			}
			return 1;
		case STRING:
			PUTC('"');
			print_string(v.string);
			PUTC('"');
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

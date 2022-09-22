#include <stdbool.h>
#include <stdio.h>
struct json_number {
	enum json_number_type {
		NUMBER_NONE, NUMBER_LONG, NUMBER_DOUBLE
	} type;
	union {
		long int long_;
		double double_;
	};
};
struct json_value {
	enum json_value_type {
		NONE, NULL_, UNDEFINED, TRUE, FALSE, NUMBER, NAN, INFINITY, ARRAY, OBJECT, STRING
	} type;
	union {
		struct json_array *array;
		struct json_object *object;
		char *string;
		struct json_number number;
	};
};
struct json_object {
	struct json_object_entry {
		char *key;
		struct json_value value;
	} *entries;
};
struct json_array {
	struct json_value *values;
};
struct json_value parse_json(FILE *fp);
bool print_json(struct json_value v, int indent);

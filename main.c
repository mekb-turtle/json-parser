#include "./json.h"
#include <stdio.h>
int main() {
	struct json_value v = parse_json(stdin);
	print_json(v, 0);
	printf("\n");
	return 0;
}

# OpenSSH config parser for C.

Works for my need.

## Supported Keywords
- User
- Host
- Include
- HostName
- IdentityFile
- IdentityAgent
- IdentitiesOnly

## Usage

```c
#include "sshconfig/parser.h"

int main() {
	FILE *f = fopen("/path/to/.ssh/config", "rb");

	Config c = parse(f);

	printf("Total Rules: %d\n", c.len);
	for (int i = 0; i < c.len; i++) {
		Rule *r = &c.rules[i];

		printf("==RULE %d==\n", i);
		printf("Total Attributes: %d\n\n", r->len);

		for (int j = 0; j < r->len; j++) {
			printf("Key=%s\tValue=%s\n", r->attributes[j].key, r->attributes[j].value);
		}
	}

	fclose(f);
	free_config(c);

	return 0;
}
```

#include "vma.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#define MAX_LEN 100

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);						\
		}							\
	} while (0)

int main(void)
{
	char command[MAX_LEN];
	arena_t *arena;

	do {
		scanf("%s", command);

		if (!strcmp(command, "ALLOC_ARENA")) {
			uint64_t arena_size;
			scanf("%ld", &arena_size);
			arena = alloc_arena(arena_size);
		} else if (!strcmp(command, "PMAP")) {
			pmap(arena);
		} else if (!strcmp(command, "ALLOC_BLOCK")) {
			uint64_t address, dim;
			scanf("%ld%ld", &address, &dim);
			alloc_block(arena, address, dim);
		} else if (!strcmp(command, "FREE_BLOCK")) {
			uint64_t address;
			scanf("%ld", &address);
			free_block(arena, address);
		} else if (!strcmp(command, "WRITE")) {
			uint64_t address, size;
			scanf("%ld%ld", &address, &size);
			int8_t *data = malloc((size + 1) * sizeof(char));
			DIE(!data, "malloc failed");
			scanf("%c", &data[0]);
			for (uint64_t i = 0; i < size; i++)
				scanf("%c", &data[i]);
			data[size] = '\0';
			write(arena, address, size, data);
			free(data);
		} else if (!strcmp(command, "READ")) {
			uint64_t address, size;
			scanf("%ld%ld", &address, &size);
			read(arena, address, size);
		} else if (!strcmp(command, "MPROTECT")) {
			int8_t *permission = malloc(MAX_LEN * sizeof(int8_t));
			DIE(!permission, "malloc failed");
			uint64_t address;
			scanf("%ld", &address);
			fgets((char *)permission, MAX_LEN, stdin);
			mprotect(arena, address, permission);
			free(permission);
		} else if (strcmp(command, "DEALLOC_ARENA")) {
			printf("Invalid command. Please try again.\n");
		}

	} while (strcmp(command, "DEALLOC_ARENA"));
	dealloc_arena(arena);
	return 0;
}

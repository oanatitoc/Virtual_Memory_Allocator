#pragma once
#include <inttypes.h>
#include <stddef.h>

/* TODO : add your implementation for doubly-linked list */
//typedef struct { } list_t;

typedef struct dll_node_t {
	void *data; /* Pentru ca datele stocate sa poata avea orice tip, folosim un
				pointer la void. */
	struct dll_node_t *prev, *next;
} dll_node_t;

typedef struct list_t {
	dll_node_t *head;
	unsigned int data_size;
	unsigned int size;
} list_t;

typedef struct miniblock_t {
	uint64_t start_address;
	size_t size;
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

typedef struct block_t {
	uint64_t start_address;
	size_t size;
	list_t *miniblock_list;
} block_t;

typedef struct {
	uint64_t arena_size;
	uint64_t arena_free_size;
	list_t *alloc_list;
	uint64_t n_miniblocks;
} arena_t;

list_t *dll_create(unsigned int data_size);
dll_node_t *dll_get_nth_node(list_t *list, unsigned int n);
void dll_add_nth_node(list_t *list, unsigned int n, const void *data);
dll_node_t *dll_remove_nth_node(list_t *list, unsigned int n);
void dll_free(list_t **pp_list);

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena);

void add_miniblock_after(arena_t *arena, dll_node_t *curr_block,
						 miniblock_t *miniblock, const uint64_t address,
						 const uint64_t size, uint64_t pos);
void add_miniblock_before(dll_node_t *curr_block, miniblock_t *miniblock,
						  const uint64_t address, const uint64_t size);
void add_block(arena_t *arena, dll_node_t *curr_block, miniblock_t *miniblock,
			   const uint64_t address, const uint64_t size, uint64_t pos);
int valid_address(arena_t *arena, const uint64_t address, const uint64_t size);
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);

void free_first(block_t *block, miniblock_t *miniblock, dll_node_t *temp);
void free_last(block_t *block, miniblock_t *miniblock, dll_node_t *temp);
void free_from_inside(arena_t *arena, dll_node_t *curr, dll_node_t *temp,
					  int nr_block, unsigned int pos);
void free_block(arena_t *arena, const uint64_t address);

char *perm_miniblock(uint8_t perm);
int check_permission(list_t *l, char r_w);

dll_node_t *get_current_block(arena_t *arena, uint64_t address);
dll_node_t *get_current_miniblock(list_t *l, uint64_t address);
void read_from_block(dll_node_t *curr_miniblock, const uint64_t address,
					 uint64_t remained_size);
void read(arena_t *arena, uint64_t address, uint64_t size);
void write_in_block(dll_node_t *curr_miniblock, const uint64_t address,
					uint64_t remained_size, int8_t *data, int index);
void write(arena_t *arena, const uint64_t address,  const uint64_t size,
		   int8_t *data);

void pmap(const arena_t *arena);
void mprotect(arena_t *arena, uint64_t address, int8_t *permission);


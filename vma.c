#include "vma.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);						\
		}							\
	} while (0)

list_t *dll_create(unsigned int data_size)
{
	list_t *l = malloc(sizeof(list_t));
	DIE(!l, "malloc failed\n");
	l->head = NULL;
	l->data_size = data_size;
	l->size = 0;
	return l;
}

dll_node_t *dll_get_nth_node(list_t *list, unsigned int n)
{
	dll_node_t *curr = list->head;
	if (!list || !list->head)
		return NULL;
	if (!list->head->next)
		return list->head;

	int index = n % list->size;
	for (int i = 0; i < index; i++)
		curr = curr->next;
	return curr;
}

void dll_add_nth_node(list_t *list, unsigned int n, const void *data)
{
	if (!list)
		exit(-1);
	// creem noul nod
	dll_node_t *new = malloc(sizeof(dll_node_t));
	DIE(!new, "malloc failed\n");
	new->data = malloc(list->data_size);
	DIE(!new->data, "malloc failed\n");
	memcpy(new->data, data, list->data_size);

	//nod auxiliar
	dll_node_t *aux;

	if (n > list->size)
		n = list->size;

	//in caz ca lista e goala
	if (!list->head) {
		new->next = NULL;
		new->prev = NULL;
		list->head = new;
	} else if (n == 0) { // se adauga pe prima pozitite dar lista nu este goala
		new->next = list->head;
		new->prev = NULL;
		list->head->prev = new;
		list->head = new;
	} else {
		aux = dll_get_nth_node(list, n - 1);
		if (n == list->size) {
			new->next = NULL;
			new->prev = aux;
			aux->next = new;
		} else {
			new->next = aux->next;
			new->prev = aux;
			aux->next->prev = new;
			aux->next = new;
		}
	}
	list->size++;
}

dll_node_t *dll_remove_nth_node(list_t *list, unsigned int n)
{
	dll_node_t *curr;
	if (!list || !list->head)
		return NULL;

	if (n >= list->size - 1)
		n = list->size - 1;
	if (n == 0) {
		curr = list->head;
		if (list->size == 1) {
			list->head = NULL;
		} else {
			list->head = curr->next;
			list->head->prev = NULL;
		}
	} else {
		curr = dll_get_nth_node(list, n);
		if (n == list->size - 1) {
			curr->prev->next = NULL;
		} else {
			curr->next->prev = curr->prev;
			curr->prev->next = curr->next;
		}
	}

	list->size--;
	if (list->size == 0)
		list->head = NULL;
	return curr;
}

void dll_free(list_t **pp_list)
{
	dll_node_t *curr;

	if (!pp_list || !(*pp_list)->head)
		return;

	while ((*pp_list)->size != 0) {
		curr = dll_remove_nth_node(*pp_list, 0);

		free(curr->data);
		free(curr);
	}

	free(*pp_list);
	*pp_list = NULL;
}

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(arena_t));
	DIE(!arena, "malloc failed\n");
	arena->arena_size = size;
	arena->arena_free_size = size;
	arena->n_miniblocks = 0;
	arena->alloc_list = dll_create(sizeof(block_t));
	return arena;
}

void dealloc_arena(arena_t *arena)
{
	dll_node_t *block = arena->alloc_list->head;
	while (block) {
		dll_node_t *miniblock = ((block_t *)block->data)->miniblock_list->head;
		while (miniblock) {
			if (((miniblock_t *)miniblock->data)->rw_buffer)
				free(((miniblock_t *)miniblock->data)->rw_buffer);
			miniblock = miniblock->next;
		}
		dll_free(&(((block_t *)(block->data))->miniblock_list));
		block = block->next;
	}

	dll_free(&arena->alloc_list);
	free(arena->alloc_list);
	free(arena);
}

/* Functie care adauga un miniblock la finalul listei dintr-un block */
void add_miniblock_last(arena_t *arena, dll_node_t *curr_block,
						miniblock_t *miniblock, const uint64_t address,
						const uint64_t size, uint64_t pos)
{
	block_t *block = (block_t *)curr_block->data;
	block->size += size;
	dll_add_nth_node(block->miniblock_list, block->miniblock_list->size,
					 miniblock);

	if (curr_block->next && (address + size ==
	    ((block_t *)curr_block->next->data)->start_address)) {
		block_t *block_next = (block_t *)curr_block->next->data;

		block->size += block_next->size;
		dll_node_t *curr_miniblock = block_next->miniblock_list->head;
		while (curr_miniblock) {
			dll_add_nth_node(block->miniblock_list,
							 block->miniblock_list->size,
							 (miniblock_t *)curr_miniblock->data);
			curr_miniblock = curr_miniblock->next;
		}

		dll_free(&block_next->miniblock_list);
		dll_node_t *removed_block = dll_remove_nth_node(arena->alloc_list, pos);
		free(((block_t *)(removed_block->data)));
		free(removed_block);
	}
}

/* FUnctie care adauga un miniblock la inceputul listei dintr-un block */
void add_miniblock_first(dll_node_t *curr_block, miniblock_t *miniblock,
						 const uint64_t address, const uint64_t size)
{
	block_t *block = (block_t *)curr_block->data;
	block->start_address = address;
	block->size += size;
	dll_add_nth_node(block->miniblock_list, 0, miniblock);
}

/*Functie prin care se adauga un miniblock care are o adresa si o dimensiune
  ce nu se leaga de celealte miniblock-uri, fiind necesara si alocarea unui
  block separat si adaugarea acestuia in lista arenei*/
void add_block(arena_t *arena, dll_node_t *curr_block, miniblock_t *miniblock,
			   const uint64_t address, const uint64_t size, uint64_t pos)
{
	block_t *block = malloc(sizeof(block_t));
	DIE(!block, "maloc failed\n");
	block->start_address = address;
	block->size = size;
	block->miniblock_list = dll_create(sizeof(miniblock_t));

	dll_add_nth_node(block->miniblock_list, 0, miniblock);
	if (curr_block && address + size <
		((block_t *)curr_block->data)->start_address)
		dll_add_nth_node(arena->alloc_list, pos - 1, block);
	else
		dll_add_nth_node(arena->alloc_list, pos, block);

	free(block);
}

/* Functie folosita pentru verificarea unei adrese valide la ALLOC_BLOCK */
int valid_address(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return 0;
	}
	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return 0;
	}

	dll_node_t *curr_block = arena->alloc_list->head;
	while (curr_block) {
		block_t *block = (block_t *)curr_block->data;

		if ((block->start_address + block->size) > address &&
			block->start_address <= address) {
			printf("This zone was already allocated.\n");
			return 0;
		}
		if ((address + size) > block->start_address &&
			address <= block->start_address) {
			printf("This zone was already allocated.\n");
			return 0;
		}

		curr_block = curr_block->next;
	}
	return 1;
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	/* Verific daca adresa si size-ul sunt valide */
	if (valid_address(arena, address, size) == 0)
		return;

	/* Aloc un nou miniblock */
	arena->n_miniblocks++;
	miniblock_t *miniblock = malloc(sizeof(miniblock_t));
	DIE(!miniblock, "maloc failed\n");
	miniblock->start_address = address;
	miniblock->size = size;
	miniblock->perm = 6;
	miniblock->rw_buffer = NULL;

	dll_node_t *curr = arena->alloc_list->head;

	/* Parcurg lista de blockuri cat timp adresa de final a block-ului curent
	e mai mica decat cea de la input */
	uint64_t pos = 1;
	while (curr && curr->next && (((block_t *)curr->data)->start_address +
		   ((block_t *)curr->data)->size) < address) {
		curr = curr->next;
		pos++;
	}

	/* Adaug noul miniblock, fie alocand un nou block, fie lipindu-l de
	block-ul curent la final sau inceput dupa caz */
	if (curr && (((block_t *)curr->data)->start_address +
	   ((block_t *)curr->data)->size) == address) {
		add_miniblock_last(arena, curr, miniblock, address, size, pos);
	} else if (curr && address + size ==
			  ((block_t *)curr->data)->start_address) {
		add_miniblock_first(curr, miniblock, address, size);
	} else {
		add_block(arena, curr, miniblock, address, size, pos);
	}
	free(miniblock);

	arena->arena_free_size -= size;
}

/* Functie care sterge primul miniblock dintr-un block */
void free_first(block_t *block, miniblock_t *miniblock, dll_node_t *temp)
{
	block->start_address += miniblock->size;
	block->size -= miniblock->size;
	temp = dll_remove_nth_node(block->miniblock_list, 0);
	free(temp->data);
	free(temp);
}

/* Functie care sterge ultimul miniblock dintr-un block */
void free_last(block_t *block, miniblock_t *miniblock, dll_node_t *temp)
{
	block->size -= miniblock->size;
	temp = dll_remove_nth_node(block->miniblock_list,
							   block->miniblock_list->size);
	free(temp->data);
	free(temp);
}

/* Functie care sterge un miniblock care se afla in interiorul listei,
   fiind necesara creearea unui nou block cu lista de miniblock-uri de
   dupa cel eliminat */
void free_from_inside(arena_t *arena, dll_node_t *curr, dll_node_t *temp,
					  int nr_block, unsigned int pos)
{
	block_t *block_curr = (block_t *)curr->data;

	temp = dll_remove_nth_node(block_curr->miniblock_list, pos);
	block_curr->size -= ((miniblock_t *)temp->data)->size;

	free(temp->data);
	free(temp);

	block_t *block = malloc(sizeof(block_t));
	DIE(!block, "maloc failed\n");
	block->miniblock_list = dll_create(sizeof(miniblock_t));
	block->size = 0;

	temp = dll_remove_nth_node(block_curr->miniblock_list, pos);
	block->start_address = ((miniblock_t *)temp->data)->start_address;
	while (temp) {
		block_curr->size -= ((miniblock_t *)temp->data)->size;
		block->size += ((miniblock_t *)temp->data)->size;
		dll_add_nth_node(block->miniblock_list, block->miniblock_list->size,
						 temp->data);
		free(temp->data);
		free(temp);
		temp = NULL;
		if (block_curr->miniblock_list->size > pos)
			temp = dll_remove_nth_node(block_curr->miniblock_list, pos);
	}
	dll_add_nth_node(arena->alloc_list, nr_block + 1, block);
	free(block);
}

void free_block(arena_t *arena, const uint64_t address)
{
	dll_node_t *curr = arena->alloc_list->head;

	int nr_block = 0;
	/* Parcurg lista de block-uri pana gasesc cel care trebuie sters */
	while (curr) {
		if (((block_t *)curr->data)->start_address +
			((block_t *)curr->data)->size > address)
			break;
		nr_block++;
		curr = curr->next;
	}
	if (!curr) {
		printf("Invalid address for free.\n");
		return;
	}

	block_t *block = (block_t *)curr->data;
	dll_node_t *temp = block->miniblock_list->head;
	miniblock_t *miniblock = (miniblock_t *)temp->data;

	/* Verific daca se sterge unicul miniblock din block => se sterge si
	block-ul din lista */
	if (block->miniblock_list->size == 1) {
		if (miniblock->start_address == address) {
			arena->arena_free_size += block->size;
			arena->n_miniblocks--;
			dll_free(&block->miniblock_list);
			curr = dll_remove_nth_node(arena->alloc_list, nr_block);
			free(curr->data);
			free(curr);
			return;
		}
		printf("Invalid address for free.\n");
		return;
	}

	/* Verific daca adresa data este adresa de inceput a unui miniblock */
	unsigned int pos = 0;
	while (temp && ((miniblock_t *)temp->data)->start_address != address) {
		temp = temp->next;
		pos++;
	}
	if (!temp) {
		printf("Invalid address for free.\n");
		return;
	}

	miniblock = (miniblock_t *)temp->data;
	arena->n_miniblocks--;
	arena->arena_free_size += miniblock->size;

	/* Sterg dupa miniblock-ul dupa caz: daca e primul, ultimul, la mijloc:
	ceea ce include creearea unui nou block */
	if (!temp->prev)
		free_first(block, miniblock, temp);
	else if (!temp->next)
		free_last(block, miniblock, temp);
	else
		free_from_inside(arena, curr, temp, nr_block, pos);
}

char *perm_miniblock(uint8_t perm);

int check_permission(list_t *l, char r_w)
{
	dll_node_t *temp = l->head;
	while (temp) {
		char *c = perm_miniblock(((miniblock_t *)temp->data)->perm);
		if (r_w == 'w' && c[1] == '-') {
			printf("Invalid permissions for write.\n");
			return 0;
		}
		if (r_w == 'r' && c[0] == '-') {
			printf("Invalid permissions for read.\n");
			return 0;
		}
		temp = temp->next;
	}
	return 1;
}

/* Parcurg lista de block-uri pana la cel pentru care s-a apelat functia
READ/WRITE */
dll_node_t *get_current_block(arena_t *arena, uint64_t address)
{
	dll_node_t *curr = arena->alloc_list->head;
	while (curr && ((block_t *)curr->data)->start_address +
		   ((block_t *)curr->data)->size < address) {
		curr = curr->next;
	}
	return curr;
}

/* Parcurg lista de miniblock-uri pana la primul care contine adresa data de
functia READ/WRITE */
dll_node_t *get_current_miniblock(list_t *l, uint64_t address)
{
	dll_node_t *curr_miniblock = l->head;
	while (((miniblock_t *)curr_miniblock->data)->start_address +
	      ((miniblock_t *)curr_miniblock->data)->size <= address) {
		curr_miniblock = curr_miniblock->next;
	}
	return curr_miniblock;
}

void read_from_block(dll_node_t *curr_miniblock, const uint64_t address,
					 uint64_t remained_size)
{
	while (curr_miniblock) {
		miniblock_t	*miniblock = ((miniblock_t *)curr_miniblock->data);
		char *string = (char *)miniblock->rw_buffer;
		/* Verific daca numarul de litere care mai trebuie citite e mai mare
		decat dimensiunea miniblock-ului curent sau nu */
		if (remained_size >= miniblock->size) {
			/* Verific daca adresa de read e cea de la inceputul
			miniblock-ului */
			if (address > miniblock->start_address) {
				for (unsigned int i = address; i < miniblock->start_address +
					 miniblock->size; i++)
					printf("%c", string[i - miniblock->start_address]);
				uint64_t end_addr = miniblock->start_address + miniblock->size;
				remained_size -= end_addr - address;
			} else {
				for (unsigned int i = 0; i < miniblock->size; i++)
					printf("%c", string[i]);
				remained_size -= miniblock->size;
			}
		} else {
			/* Verific daca adresa de read e cea de la inceputul
			miniblock-ului */
			if (address > miniblock->start_address) {
				for (unsigned int i = address; i < address + remained_size; i++)
					printf("%c", string[i - miniblock->start_address]);
			} else {
				for (unsigned int i = 0; i < remained_size; i++)
					printf("%c", string[i]);
			}
			remained_size -= remained_size;
		}
		if (remained_size == 0) {
			printf("\n"); return;
		}
		curr_miniblock = curr_miniblock->next;
	}
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	if (address >= arena->arena_size) {
		printf("Invalid address for read.\n");
		return;
	}

	dll_node_t *curr_block = get_current_block(arena, address);

	/* Daca nu am gasit block-ul care sa contina adresa sau adresa de read e
	mai mica decat adresa primului block, afisez mesaj de eroare */
	if (!curr_block || ((block_t *)curr_block->data)->start_address > address) {
		printf("Invalid address for read.\n");
		return;
	}
	block_t *block = (block_t *)curr_block->data;

	/* Verific daca toate miniblock-urile au permisiuni de read */
	if (!check_permission(block->miniblock_list, 'r'))
		return;

	/* Aflu numarul de caractere care pot fi citite din  block */
	uint64_t remained_size = size;
	if (block->start_address + block->size - address < size) {
		printf("Warning: size was bigger than the block size. ");
		printf("Reading %ld characters.\n",
			   block->start_address + block->size - address);
		remained_size = block->start_address + block->size - address;
	}

	/* Aflu primul miniblock de unde incepe scrierea */
	dll_node_t *curr_miniblock = get_current_miniblock(block->miniblock_list,
													   address);
	read_from_block(curr_miniblock, address, remained_size);
}

/* Functia care scrie in block-ul curent */
void write_in_block(dll_node_t *curr_miniblock, const uint64_t address,
					uint64_t remained_size, int8_t *data, int index)
{
	while (curr_miniblock) {
		miniblock_t *miniblock = (miniblock_t *)curr_miniblock->data;
		char *string = malloc((miniblock->size + 1) * sizeof(char));
		DIE(!string, "malloc failed\n");
		miniblock->rw_buffer = calloc((miniblock->size + 1), sizeof(char));
		DIE(!miniblock->rw_buffer, "malloc failed\n");

		/* Verific daca tebuie sa completez tot spatiul pana la finalul
		miniblock-ului sau nu */
		if (remained_size >= miniblock->size) {
			/* Verific daca adresa este de la inceputul miniblock-ului */
			if (address > miniblock->start_address) {
				for (uint64_t i = address; i < miniblock->start_address +
					 miniblock->size; i++) {
					string[i - miniblock->start_address] = (char)(data[index]);
					index++;
				}
			} else {
				for (unsigned int i = 0; i < miniblock->size; i++) {
					string[i] = (char)(data[index]);
					index++;
				}
			}
			string[miniblock->size] = '\0';
			memcpy(miniblock->rw_buffer, string, miniblock->size + 1);
			remained_size -= miniblock->size;
		} else {
			for (unsigned int i = 0; i < remained_size; i++) {
				string[i] = (char)(data[index]);
				index++;
			}
			string[remained_size] = '\0';
			memcpy(miniblock->rw_buffer, string, remained_size + 1);
			remained_size -= remained_size;
		}

		free(string);
		if (remained_size == 0)
			return;
		curr_miniblock = curr_miniblock->next;
	}
}

void write(arena_t *arena, const uint64_t address, const uint64_t size,
		   int8_t *data)
{
	if (address >= arena->arena_size) {
		printf("Invalid address for write.\n");
		return;
	}

	dll_node_t *curr_block = get_current_block(arena, address);

	/* Daca nu am gasit block-ul care sa contina adresa sau adresa de write e
	mai mica decat adresa primului block, afisez mesaj de eroare */
	if (!curr_block || ((block_t *)curr_block->data)->start_address > address) {
		printf("Invalid address for write.\n");
		return;
	}
	block_t *block = (block_t *)curr_block->data;

	/* Verific daca toate miniblock-urile au permisiuni de write */
	if (!check_permission(block->miniblock_list, 'w'))
		return;

	/* Aflu numarul de caractere care pot fi scrise in block */
	uint64_t remained_size = size;
	if (block->start_address + block->size - address < size) {
		printf("Warning: size was bigger than the block size. ");
		printf("Writing %ld characters.\n",
			   block->start_address + block->size - address);
		remained_size = block->size;
	}

	/* Aflu primul miniblock de unde incepe scrierea */
	dll_node_t *curr_miniblock = get_current_miniblock(block->miniblock_list,
													   address);

	/* Indexul caracterului citit din "data" */
	int index = 0;

	write_in_block(curr_miniblock, address, remained_size, data, index);
}

char *perm_miniblock(uint8_t perm)
{
	if (perm == 7)
		return "RWX";
	if (perm == 6)
		return "RW-";
	if (perm == 5)
		return "R-E";
	if (perm == 4)
		return "R--";
	if (perm == 3)
		return "-WE";
	if (perm == 2)
		return "-W-";
	if (perm == 1)
		return "--E";
	return "---";
}

void pmap(const arena_t *arena)
{
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	printf("Free memory: 0x%lX bytes\n", arena->arena_free_size);
	printf("Number of allocated blocks: %d\n", arena->alloc_list->size);
	printf("Number of allocated miniblocks: %ld\n", arena->n_miniblocks);

	dll_node_t *curr_block = arena->alloc_list->head;

	for (unsigned int i = 0; i < arena->alloc_list->size; i++) {
		block_t *block = (block_t *)curr_block->data;
		printf("\n");
		printf("Block %d begin\n", i + 1);
		printf("Zone: 0x%lX - 0x%lX\n", block->start_address,
			   block->start_address + block->size);

		dll_node_t *curr_miniblock = block->miniblock_list->head;
		for (unsigned int j = 0; j < block->miniblock_list->size; j++) {
			miniblock_t *miniblock = (miniblock_t *)curr_miniblock->data;
			char *perm = perm_miniblock(miniblock->perm);
			printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| %s\n", j + 1,
				   miniblock->start_address, miniblock->start_address +
				   miniblock->size, perm);
			curr_miniblock = curr_miniblock->next;
		}
		curr_block = curr_block->next;
		printf("Block %d end\n", i + 1);
	}
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	/* Parcug lista de block-uri pana la cel la care se afla adresa
	miniblock-ului pentru care s-a apelat functia */
	dll_node_t *curr_block = arena->alloc_list->head;
	block_t *block;
	if (curr_block)
		block = (block_t *)curr_block->data;
	while (curr_block && block->start_address + block->size < address) {
		curr_block = curr_block->next;
		if (curr_block)
			block = (block_t *)curr_block->data;
	}
	if (!curr_block) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	/* Parcurg lista de miniblock-uri pana ce ajung la miniblock-ul pentru care
	s-a apelat functia */
	dll_node_t *curr_miniblock = block->miniblock_list->head;
	miniblock_t *miniblock = (miniblock_t *)curr_miniblock->data;
	while (curr_miniblock && miniblock->start_address != address) {
		curr_miniblock = curr_miniblock->next;
		if (curr_miniblock)
			miniblock = (miniblock_t *)curr_miniblock->data;
	}
	if (!curr_miniblock) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	uint8_t perm = 0;
	if (strstr((char *)permission, "PROT_NONE"))
		perm = 0;
	if (strstr((char *)permission, "PROT_READ"))
		perm += 4;
	if (strstr((char *)permission, "PROT_WRITE"))
		perm += 2;
	if (strstr((char *)permission, "PROT_EXEC"))
		perm += 1;

	((miniblock_t *)curr_miniblock->data)->perm = perm;
}

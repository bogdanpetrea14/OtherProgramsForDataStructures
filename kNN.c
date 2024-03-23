#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#define INF 999999999

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);					\
		}							\
	} while (0)

typedef struct node node;

/*
 *	Structura pentru un nod din arbore.
 *	Contine un pointer catre nodul parinte, un pointer catre nodul din stanga,
 *	un pointer catre nodul din dreapta, nivelul nodului si un vector de date.
*/

struct node {
	node *left;
	node *right;
	node *parent;
	int level;
	int *data;
};

/*
 *	Structura pentru coordonate.
 *	Contine un pointer catre radacina arborelui si dimensiunea datelor.
*/

typedef struct {
	node *root;
	int data_size;
} coords;

/*
	Functie pentru initializarea coordonatelor(grafului care va fi construit)
*/

coords *init_coords(void)
{
	coords *c = malloc(sizeof(coords));
	DIE(!c, "Cannot alloc memory for the coords!\n");

	c->root = NULL;
	c->data_size = 0;
	return c;
}

/*
 *		1 - LOAD
 *		2 - NN
 *		3 - RS
 *		0 - EXIT
*/

int transform_command(char *command)
{
	if (!strcmp(command, "LOAD\0"))
		return 1;
	if (!strcmp(command, "NN\0"))
		return 2;
	if (!strcmp(command, "RS\0"))
		return 3;
	if (!strcmp(command, "EXIT\0"))
		return 0;
	return -1;
}

/*
	Functie folosita pentru a incarca datele. Se citesc din
	filename numarul de puncte si in cate dimensiuni sunt
	acestea, apoi se citesc punctele si se construieste arborele.
	Prima data se aloca noul nod, se citesc intr-un vector de int
	datele si se copiaza in nod, apoi se cauta unde in arbore pozitia
	unde ar trebui sa vina acel nod in functie de coordonate si nivelul
	pe care se afla, dupa care se insereaza nodul in arbore, fiind
	o reinterpretarare a BST-ului.

*/

void load(char *filename, coords *coord)
{
	int n, k;
	int *dat;

	FILE *f = fopen(filename, "r");
	if (!f) {
		printf("Cannot open file %s\n", filename);
		return;
	}

	fscanf(f, "%d%d", &n, &k);
	coord->data_size = k * sizeof(int);
	dat = malloc(coord->data_size);
	DIE(!dat, "Cannot alloc memory for the data!\n");

	for (int i = 0; i < k; i++)
		fscanf(f, "%d", &dat[i]);

	coord->root = malloc(sizeof(node));
	DIE(!coord->root, "Cannot alloc memory for the root!\n");

	coord->root->data = malloc(coord->data_size);
	DIE(!coord->root->data, "Cannot alloc memory for the root data!\n");

	memcpy(coord->root->data, dat, coord->data_size);
	coord->root->left = NULL;
	coord->root->right = NULL;
	coord->root->parent = NULL;
	coord->root->level = 1;

	node *cur = coord->root;

	for (int i = 1; i < n; i++) {
		for (int j = 0; j < k; j++)
			fscanf(f, "%d", &dat[j]);
		node *new_node = malloc(sizeof(node));
		DIE(!new_node, "Cannot alloc memory for the node!\n");
		new_node->data = malloc(coord->data_size);
		DIE(!new_node->data, "Cannot alloc memory for the node data!\n");

		memcpy(new_node->data, dat, coord->data_size);

		node *parent = NULL;
		while (cur) {
			parent = cur;
			if (dat[cur->level % k] < cur->data[cur->level % k])
				cur = cur->left;
			else
				cur = cur->right;
		}

		new_node->parent = parent;
		new_node->left = NULL;
		new_node->right = NULL;
		new_node->level = (parent->level) + 1;

		if (dat[parent->level % k] < parent->data[parent->level % k])
			parent->left = new_node;
		else
			parent->right = new_node;

		cur = coord->root;
	}

	free(dat);
	fclose(f);
}

/*
	Functie folosita pentru eliberarea memoriei ocupate de arbore.
	Se parcurge arborele in postordine si se elibereaza memoria
	ocupata de fiecare nod si de datele acestuia.
*/

void free_tree(node *root)
{
	if (!root)
		return;

	free_tree(root->left);
	free_tree(root->right);

	free(root->data);
	free(root);
}

void free_coords(coords *coord)
{
	free_tree(coord->root);
	coord->root = NULL;
	coord->data_size = 0;
}

/*
	Functie folosita pentru a calcula distanta dintre 2 puncte.
	Se parcurg cele 2 puncte si se calculeaza distanta dintre
	acestea, apoi se returneaza distanta.
*/

double calculate_distance(int *point1, int *point2, int dimensions)
{
	double distance = 0.0;
	for (int i = 0; i < dimensions; i++)
		distance += (point1[i] - point2[i]) * (point1[i] - point2[i]);
	return sqrt(distance);
}

/*
	Functie folosita pentru a gasi cel mai apropiat vecin al unui
	punct. Se parcurge arborele si se calculeaza distanta dintre
	punctul curent si punctul cautat, daca distanta este mai mica
	decat distanta minima gasita pana acum, se actualizeaza distanta
	minima si se retine nodul curent ca fiind cel mai apropiat vecin.
	Se cauta in continuare in ambii subarbori, in functie de
	coordonatele punctului cautat si de coordonata nodului curent.
	Pentru acel ok, daca este 1, inseamna ca s-a cautat o data cel
	mai apropiat vecin si se cauta in continuare pentru puncte
	aflate la aceeasi distanta, iar daca este 0, inseamna ca nu
	s-a cautat inca cel mai apropiat vecin.
*/

void find_nearest_neighbor(node *current, int *target, int dimensions,
						   node **nearest, double *min_distance, int ok)
{
	if (!current)
		return;

	float distance = calculate_distance(target, current->data, dimensions);
	if (distance < *min_distance) {
		*min_distance = distance;
		*nearest = current;
	}

	int lvl = current->level % dimensions;
	if (target[lvl] >= current->data[lvl])
		find_nearest_neighbor(current->right, target,
							  dimensions, nearest, min_distance, ok);
	else
		find_nearest_neighbor(current->left, target,
							  dimensions, nearest, min_distance, ok);

	float dist = (target[lvl] - current->data[lvl]);
	if (dist < 0)
		dist = -dist;
	if (dist <= *min_distance) {
		if (target[lvl] >= current->data[lvl])
			find_nearest_neighbor(current->left, target, dimensions,
								  nearest, min_distance, ok);
		else
			find_nearest_neighbor(current->right, target, dimensions,
								  nearest, min_distance, ok);
	}

	if (ok && distance == *min_distance) {
		for (int i = 0; i < dimensions; i++)
			printf("%d ", current->data[i]);
		printf("\n");
	}
}

/*
	Functie folosita pentru a gasi cel mai apropiat vecin al unui
	punct. Se apeleaza functia find_nearest_neighbor pentru a gasi
	cel mai apropiat vecin si se actualizeaza coordonatele punctului
	cautat cu coordonatele celui mai apropiat vecin.
*/

void nearest_neighbor(coords *coord, int *target, int dimensions)
{
	if (!coord || !coord->root)
		return;
	node *nearest = NULL;
	double min_distance = INF;
	int ok = 0;
	find_nearest_neighbor(coord->root, target, dimensions,
						  &nearest, &min_distance, ok);
	ok = 1;
	nearest = NULL;
	find_nearest_neighbor(coord->root, target, dimensions,
						  &nearest, &min_distance, ok);
}

/*
	Functie folosita pentru a verifica daca un punct se afla
	intr-un anumit interval. Se verifica pentru fiecare coordonata
	daca se afla in intervalul dat si se returneaza 1 daca se afla
	si 0 in caz contrar.
*/

int is_within_range(int value, int start, int end)
{
	return (value >= start && value <= end);
}

/*
	Functie folosita pentru a cauta punctele dintr-un interval.
	Se parcurge arborele si se verifica daca fiecare punct se
	afla in intervalul dat, daca se afla, se afiseaza punctul.
	Se cauta in continuare in ambii subarbori, in functie de
	coordonatele punctului cautat si de coordonata nodului curent.
*/

void range_search(node *current, int *start, int *end, int dimensions)
{
	if (!current)
		return;

	short lvl = current->level % dimensions;

	if (start[lvl] <= current->data[lvl])
		range_search(current->left, start, end, dimensions);

	if (end[lvl] >= current->data[lvl])
		range_search(current->right, start, end, dimensions);

	int is_in_range = 1;
	for (int i = 0; i < dimensions; i++) {
		if (!is_within_range(current->data[i], start[i], end[i])) {
			is_in_range = 0;
			break;
		}
	}

	if (is_in_range) {
		for (int i = 0; i < dimensions; i++)
			printf("%d ", current->data[i]);
		printf("\n");
	}
}

void perform_range_search(coords *coord, int *start,
						  int *end, int dimensions)
{
	if (!coord || !coord->root)
		return;

	range_search(coord->root, start, end, dimensions);
}

int main(void)
{
	char line[255], command[255], trash[255], filename[255];
	coords *cord = init_coords();
	int ok, *dat;
	do {
		fgets(line, 255, stdin);
		if (sscanf(line, "%s", command) != 1)
			printf("ERROR\n");
		ok = transform_command(command);
		switch (ok) {
		case 1:

			if (sscanf(line, "%s %s", trash, filename) != 2)
				printf("ERROR\n");
			load(filename, cord);

		break;
		case 2:

			if (sscanf(line, "%s", trash) != 1)
				printf("ERROR\n");
			dat = malloc(sizeof(int) * cord->data_size);
			if (cord->data_size / sizeof(int) == 2) {
				if (sscanf(line, "%s%d%d", trash, &dat[0], &dat[1]) != 3)
					printf("ERROR\n");
			} else {
				if (sscanf(line, "%s%d%d%d", trash, &dat[0],
						   &dat[1], &dat[2]) != 4)
					printf("ERROR\n");
			}
			nearest_neighbor(cord, dat, cord->data_size / sizeof(int));
			free(dat);

		break;
		case 3:
		if (cord->data_size / sizeof(int) == 2) {
			dat = malloc(sizeof(int) * 4);
			if (sscanf(line, "%s%d%d%d%d", trash, &dat[0], &dat[2],
					   &dat[1], &dat[3]) != 5)
				printf("ERROR\n");
			perform_range_search(cord, dat, dat + 2, 2);
			free(dat);
		} else {
			dat = malloc(sizeof(int) * 6);
			if (sscanf(line, "%s%d%d%d%d%d%d", trash, &dat[0],
					   &dat[3], &dat[1], &dat[4], &dat[2], &dat[5]) != 7)
				printf("ERROR\n");
			perform_range_search(cord, dat, dat + 3, 3);
			free(dat);
		}

		break;
		case 0:
			free_coords(cord);
			free(cord);
		break;
		}
	} while (ok);
	return 0;
}

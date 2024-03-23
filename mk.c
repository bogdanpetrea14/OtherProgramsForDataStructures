#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define DIE(assertion, call_description)                                       \
	do {                                                                       \
		if (assertion) {                                                       \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                 \
			perror(call_description);                                          \
			exit(errno);                                                       \
		}                                                                      \
	} while (0)

#define MAX_LENGTH 100
#define MAX_COM 255

/*
*	Structura pentru un cuvant din dictionar.
*	Contine cuvantul si frecventa acestuia.
*/

typedef struct {
	char word[MAX_LENGTH];
	int frequency;
} words;

/*
*	Structura pentru dictionar.
*	Contine un vector de cuvinte, un vector de frecvente pentru fiecare litera
*	din alfabet, capacitatea dictionarului si numarul de cuvinte din dictionar.
*/

typedef struct {
	words *entry;
	int alf[26];
	int capacity;
	int count;
} dictionary;

/*
 *		1 - INSERT
 *		2 - LOAD
 *		3 - REMOVE
 *		4 - AUTOCORRECT
 *		5 - AUTOCOMPLETE
 *		0 - EXIT
*/

int transform_command(char *command)
{
	if (!strcmp(command, "INSERT\0"))
		return 1;
	if (!strcmp(command, "LOAD\0"))
		return 2;
	if (!strcmp(command, "REMOVE\0"))
		return 3;
	if (!strcmp(command, "AUTOCORRECT\0"))
		return 4;
	if (!strcmp(command, "AUTOCOMPLETE\0"))
		return 5;
	if (!strcmp(command, "EXIT\0"))
		return 0;
	return -1;
}

/*
	Functie pentru initierea dictionarului.
*/

dictionary *init_dictionary(void)
{
	dictionary *d = malloc(sizeof(dictionary));
	DIE(!d, "Cannot alloc memory for the dictionary!\n");

	d->capacity = 10;
	d->count = 0;
	for (int i = 0; i < 26; i++)
		d->alf[i] = 0;
	d->entry = malloc(d->capacity * sizeof(words));
	DIE(!d->entry, "cannot alloc memory");
	return d;
}

/*
	Functie pentru inserarea unui cuvant in dictionar. Este cautat in lista
	cuvantul, iar daca exista se incrementeaza frecventa, altfel se insereaza
	cuvantul in ordine alfabetica.
*/

void insert_word(char *word, dictionary *d)
{
	int i;

	for (i = 0; i < d->count; i++)
		if (strcmp(d->entry[i].word, word) >= 0)
			break;

	if (i < d->count && !strcmp(d->entry[i].word, word)) {
		d->entry[i].frequency++;
		return;
	}

	if (d->count == d->capacity) {
		d->capacity *= 2;
		d->entry = realloc(d->entry, d->capacity * sizeof(words));
		DIE(!d->entry, "Realloc failed");
	}

	memmove(&d->entry[i + 1], &d->entry[i], (d->count - i) * sizeof(words));
	strcpy(d->entry[i].word, word);
	d->entry[i].frequency = 1;
	d->count++;
	int pos = word[0] - 'a';
	d->alf[pos]++;
}

/*
	Functie pentru incarcarea unui fisier in dictionar. Se citeste
	fiecare cuvant pe rand, iar apoi se insereaza in dictionar, apeland
	functia insert_word.
*/

void load_file(char *filename, dictionary *d)
{
	FILE *file = fopen(filename, "r");
	if (!file) {
		printf("Cannot open the file");
		return;
	}

	char word[MAX_LENGTH];
	while (fscanf(file, "%s", word) == 1)
		insert_word(word, d);
	fclose(file);
}

/*
	Functie pentru stergerea unui cuvant din dictionar. Se cauta cuvantul
	in lista, iar daca se gaseste se muta toate cuvintele de dupa cuvantul
	eliminat cu o pozitie in stanga.
*/

void remove_word(char *word, dictionary *d)
{
	int i;
	for (i = 0; i < d->count; i++) {
		if (!strcmp(d->entry[i].word, word)) {
			memmove(&d->entry[i], &d->entry[i + 1],
					(d->count - i - 1) * sizeof(words));
			d->count--;
			return;
		}
	}
}

/*
	Functie pentru numararea caracterelor diferite dintre doua cuvinte.
	Se parcurg cele doua cuvinte si se incrementeaza un contor de fiecare
	data cand se gaseste un caracter diferit. Daca cele doua cuvinte sunt
	diferite, se ignora, caci nu se poate face autocorrect pe cuvinte
	mai lungi sau mai scurte intre ele.
*/

int count_dif_chr(char *w1, char *w2)
{
	int count = 0;
	if (strlen(w1) != strlen(w2))
		return 99999;
	for (unsigned long i = 0; i < strlen(w1); i++)
		if (w1[i] != w2[i])
			count++;
	return count;
}

/*
	Functie pentru autocorrect. Se parcurge lista de cuvinte si se verifica
	daca numarul de caractere diferite dintre cuvantul dat si cuvantul din
	lista este mai mic sau egal cu k. Daca este, se afiseaza cuvantul.
*/

void autocorrect(char *word, dictionary *d, int k)
{
	int i, ok = 0;
	for (i = 0; i < d->count; i++)
		if (count_dif_chr(word, d->entry[i].word) <= k) {
			printf("%s\n", d->entry[i].word);
			ok = 1;
		}

	if (!ok)
		printf("No words found\n");
}

/*
	Functie pentru autocomplete. Se extrage posibila pozitie a
	cuvantului in dictionar, adica se calculeaza cate cuvinte sunt
	inaintea lui in dictionar, iar dupa cate cuvinte sunt dupa el.
	(Adica, se ia prima litera, spre exemplu 'b', se verifica cate
	cuvinte sunt cu 'a' inaintea lui, iar apoi cate cu 'c' dupa el).
	In aceasta lista, se cauta cuvintele pentru autocomplete, iar
	daca se gaseste un cuvant care incepe cu cuvantul dat, se afiseaza.
	Calculam si cel mai frecvent cuvant, dar si cel mai scurt`cuvant.
*/

void autocomplete(char *word, dictionary *d, int k)
{
	int i, j;
	words *suggestion = malloc(sizeof(words) * d->count);
	DIE(!suggestion, "Malloc failed again");

	int s_count = 0;
	int which = word[0] - 'a';
	int cont = 0, end_cont = 0;
	for (j = 0; j < which - 1; j++)
		cont += d->alf[j];
	if (which != 25) {
		for (j = 0; j < which + 1; j++)
			end_cont += d->alf[j];
	} else {
		end_cont = d->count;
	}

	for (i = cont; i < end_cont; i++)
		if (!strncmp(d->entry[i].word, word, strlen(word))) {
			strcpy(suggestion[s_count].word, d->entry[i].word);
			suggestion[s_count].frequency = d->entry[i].frequency;
			s_count++;
		}

	if (!s_count) {
		printf("No words found\n");
		return;
	}

	if (k == 0 || k == 1)
		printf("%s\n", suggestion[0].word);

	if (k == 0 || k == 2) {
		int short_ind = 0;
		int short_len = strlen(suggestion[0].word);
		for (i = 1; i < s_count; i++) {
			int cur_len = strlen(suggestion[i].word);
			if (cur_len < short_len) {
				short_ind = i;
				short_len = cur_len;
			}
		}
		printf("%s\n", suggestion[short_ind].word);
	}

	if (k == 0 || k == 3) {
		int most_fr_ind = 0;
		int most_fr = suggestion[0].frequency;
		for (i = 1; i < s_count; i++) {
			int cur_fr = suggestion[i].frequency;
			if (cur_fr > most_fr) {
				most_fr_ind = i;
				most_fr = cur_fr;
			}
		}
		printf("%s\n", suggestion[most_fr_ind].word);
	}

	free(suggestion);
}

int main(void)
{
	dictionary *d = init_dictionary();

	char command[MAX_COM], trash[MAX_COM], word[MAX_COM], line[MAX_COM];
	int which, k;
	do {
		fgets(line, MAX_COM, stdin);
		if (sscanf(line, "%s", command) != 1)
			printf("ERROR\n");
		which = transform_command(command);
		switch (which) {
		case 1:

		if (sscanf(line, "%s%s", trash, word) != 2)
			printf("ERROR\n");
		insert_word(word, d);

		break;
		case 2:

		if (sscanf(line, "%s%s", trash, word) != 2)
			printf("ERROR\n");
		load_file(word, d);

		break;
		case 3:

		if (sscanf(line, "%s%s", trash, word) != 2)
			printf("ERROR\n");
		remove_word(word, d);

		break;
		case 4:

		if (sscanf(line, "%s%s%d", trash, word, &k) != 3)
			printf("ERROR\n");
		autocorrect(word, d, k);

		break;
		case 5:

		if (sscanf(line, "%s%s%d", trash, word, &k) != 3)
			printf("ERROR\n");
		autocomplete(word, d, k);

		break;
		case 0:

		free(d->entry);
		free(d);

		break;
		}
	} while (which);
	return 0;
}

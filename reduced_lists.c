#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* Needed only for search, so can be easily removed */

#include "structures.h"
#include "service_func.h"
#include "reduced_lists.h"

#define BUF_SIZE     64
#define CHAR_SIZE    sizeof(char)


char *strread_file(FILE *fn, int *err_code, long *symb_number)
{
        int ch;
        int buf_count = 0;
        unsigned int mult = 1;
        unsigned long offset = 0;
        char *storage_link = NULL;
        storage_link = (char *) malloc(BUF_SIZE * sizeof(char));

        if (storage_link == NULL)
        {
                printf("No available memory\n");
                *err_code = -1;
                return NULL;
        }

        storage_link[0] = '\0';

        while ((ch=fgetc(fn)) != '\n')
        {
                if (buf_count > (BUF_SIZE-1))
                {
                        buf_count = 0;
                        mult++;
                        storage_link = (char *) realloc(storage_link, BUF_SIZE * mult * sizeof(char));

                        if (storage_link == NULL)
                        {
                                printf("No available memory\n");
                                *err_code = -1;
                                return NULL;
                        }
                }

                if (ch == EOF)
                {
                        /*printf("EOF REACHED\n");*/
                        *err_code = -2;
                        break;
                }

                storage_link[offset] = ch;
                (*symb_number)++;
                offset++;
                buf_count++;
        }

        if (buf_count > (BUF_SIZE-1))
        {
                storage_link = (char *) realloc
                (storage_link, BUF_SIZE * mult * CHAR_SIZE + CHAR_SIZE);

                if (storage_link == NULL)
                {
                        printf("No available memory\n");
                        return NULL;
                }
        }

        *(storage_link + offset) = '\0';
        return storage_link;
}


void bidir_lst_init(Bidir_lst *listname)
{
	listname -> head = NULL;
	listname -> tail = NULL;
}


void bidir_lst_append(Bidir_lst *listname, char *data, long lenn)
{
	struct bidir_item_lst *new_item;
	new_item = (struct bidir_item_lst *) malloc(sizeof(struct bidir_item_lst));

	if (listname -> head == NULL && listname -> tail == NULL)
	{	
		new_item -> body = data;
		new_item -> prev = NULL;
		new_item -> next = NULL;
		new_item -> leng = lenn;
		listname -> head = new_item;
		listname -> tail = new_item;
		return;
	}

	new_item -> body = data;
	new_item -> prev = listname -> tail;
	new_item -> next = NULL;
	new_item -> leng = lenn;
	listname -> tail -> next = new_item;
	listname -> tail = new_item;
}


/*void bidir_lst_swap(struct bidir_item_lst *el_1, struct bidir_item_lst *el_2)
{
	char *tmp = el_2 -> body;
	long tmp_2 = el_2 -> leng;
	el_2 -> body = el_1 -> body;
	el_2 -> leng = el_1 -> leng;
	el_1 -> body = tmp;
	el_1 -> leng = tmp_2;
}*/


void bidir_lst_erase(Bidir_lst *listname)
{
	struct bidir_item_lst *tmp = listname -> head;
	struct bidir_item_lst *delet = NULL;

	while (tmp != NULL)
	{
		delet = tmp;
		tmp = tmp -> next;
		free(delet -> body);
		free(delet);
	}

	listname -> head = NULL;
	listname -> tail = NULL;
}


void bidir_lst_del_one(Bidir_lst *listname, long elem_num)
{
	/* If elem_num == -1, deletes last element; do not use this w/ empty list */

	struct bidir_item_lst *victim = listname -> head;
	long curr_num = 1;

	if (elem_num == -1)
	{
		if (listname -> head == listname -> tail)	/* If list contains only 1 element */
		{
			listname -> head = listname -> tail = NULL;
			free(victim -> body);
			free(victim);
			return;
		}

		victim = listname -> tail;
		listname -> tail = listname -> tail -> prev;
		listname -> tail -> next = NULL;
		free(victim -> body);
		free(victim);
		return;
	}

	while (curr_num != elem_num)
	{
		victim = victim -> next;
		curr_num++;
	}

	if (victim == listname -> head && victim == listname -> tail)
	{
		free(victim -> body);
		free(victim);
		listname -> head = NULL;
		listname -> tail = NULL;
		return;
	}

	if (victim == listname -> head)
	{
		victim -> next -> prev = NULL;
		listname -> head = victim -> next;		
		free(victim -> body);
		free(victim);
		return;
	}

	if (victim == listname -> tail)
	{
		victim -> prev -> next = NULL;
		listname -> tail = victim -> prev;	
		free(victim -> body);
		free(victim);
		return;
	}

	victim -> prev -> next = victim -> next;
	victim -> next -> prev = victim -> prev;
	free(victim -> body);
	free(victim);
}


void bidir_lst_readall(Bidir_lst *listname)
{
	struct bidir_item_lst *tmp = listname -> head;
	long max_leng_element, field_width;

	if (tmp == NULL)
	{
		fprintf(stderr, "Nothing to read: Empty list.\n");
		return;
	}

	max_leng_element = tmp -> leng;

	while (tmp != NULL)
	{
		if (tmp->leng > max_leng_element)
			max_leng_element = tmp -> leng;

		tmp = tmp -> next;
	}

	field_width = digit_number(max_leng_element);

	tmp = listname -> head;
	printf("list containing:\n");

	while (tmp != NULL)
	{
		int offset = 0;
		unsigned char elem = tmp -> body[offset];
		long difference = field_width - digit_number(tmp -> leng);

		printf(" ");

		while (difference--)
			printf(" ");

		printf("%ld | <", tmp -> leng);

		while (elem != '\0')
		{
			putchar((int) elem);
			offset++;
			elem = tmp -> body[offset];
		}
	
		printf(">");
		printf("\n");
		tmp = tmp -> next;
	}
}


int bidir_lst_search(Bidir_lst *list, char *pattern)
{
	/* Defines if <pattern> is among list elements. On success, returns
	 * number of that element. Else, returns -1.
	 */

	struct bidir_item_lst *tmp = list -> head;
	int result = 1;

	while (tmp != NULL)
	{
		if (!strcmp(tmp->body, pattern))
			return result;

		result++;
		tmp = tmp -> next;
	}

	return -1;
}


char *bidir_lst_getbylen(Bidir_lst *list, long pattern)
{
	/* Tries to return element of the <list>, (.leng) element
	 * of which is equal to the <pattern>. On success,
	 * returns pointer to that containts; if not found,
	 * returns NULL.
	 */

	struct bidir_item_lst *tmp = list -> head;

	while (tmp != NULL)
	{
		if (tmp -> leng == pattern)
			return tmp -> body;

		tmp = tmp -> next;
	}

	return NULL;
}


int bidir_lst_length(Bidir_lst *list)
{
	/* Just calculates number of elements in <list>. */

	struct bidir_item_lst *tmp = list -> head;
	int num = 0;

	while (tmp != NULL)
	{
		num++;
		tmp = tmp -> next;
	}

	return num;
}


int read_to_list(FILE *filename, Bidir_lst *list, long *gl_str_num)
{
    char *file_string = NULL;

    while (1)
    {
        int err = 0;
        long symb_number = 0;

        file_string = strread_file(filename, &err, &symb_number);

        if (err == -1)
        {
            printf("Some problems with memory allocating.\n");
            return -1;
        }

        if (err == -2)
        {
            free(file_string);
            break;
        }

        bidir_lst_append(list, file_string, symb_number);
        (*gl_str_num)++;
    }

    return 0;
}


int write_from_list(FILE *filename, Bidir_lst *list)
{
    struct bidir_item_lst *current = list -> head;

    while (current != NULL)
    {
        int offset = 0;
        unsigned char elem = current -> body[offset];

        while (offset < (current -> leng))
        {
            if ((elem == '\r') && (current -> body[offset+1] == '\0'))
            {
                offset++;           /* This for [dos] files */
                elem = current -> body[offset];
                continue;
            }

            if (fputc((int) elem, filename) == EOF)
            {
                printf("Writing error.\n");
                return 1;
            }

            offset++;
            elem = current -> body[offset];
        }

        if (current != (list -> tail))
        {
            if (fputc((int) '\n', filename) == EOF)
            {
                printf("Writing error.\n");
                return 3;
            }
        }

        current = current -> next;
    }

    fputc((int) '\n', filename);
    return 0;
}


char *list_to_string(Bidir_lst *list)
{
	/* Do not use with empty list */

	struct bidir_item_lst *tmp = list -> head -> next;
	char *result;
	long realloc_size = 0;
	long offset = 0;
	char cc;

	result = (char *) malloc(CHAR_SIZE * list -> head -> leng + CHAR_SIZE);

	while ((cc = list -> head -> body[offset]) != '\0')
	{
		result[offset] = cc;
		offset++;
	}

	while (tmp != NULL)
	{
		char cc;
		long local_offset = 0;

		realloc_size = (tmp -> next == NULL) ? (offset + tmp -> leng + 1) :
		(offset + tmp -> leng);
		result = (char *) realloc(result, CHAR_SIZE * realloc_size);

		while ((cc = tmp -> body[local_offset]) != '\0')
		{
			result[offset] = cc;
			local_offset++;
			offset++;
		}

		tmp = tmp -> next;
	}

	result[offset] = '\0';
	return result;
}

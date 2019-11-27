#ifndef __REDUCED_LISTS_H__
#define __REDUCED_LISTS_H__

/*struct bidir_item_lst
{
        char *body;
        struct bidir_item_lst *prev;
        struct bidir_item_lst *next;
	long leng;
};


typedef struct
{
        struct bidir_item_lst *head;
        struct bidir_item_lst *tail;

} Bidir_lst;*/


char *strread_file(FILE *fn, int *err_code, long *symb_number);

void bidir_lst_init(Bidir_lst *listname);

void bidir_lst_append(Bidir_lst *listname, char *data, long lenn);

/*void bidir_lst_swap(struct bidir_item_lst *el_1, struct bidir_item_lst *el_2);*/

void bidir_lst_erase(Bidir_lst *listname);

void bidir_lst_del_one(Bidir_lst *listname, long elem_num);

void bidir_lst_readall(Bidir_lst *listname);

int bidir_lst_search(Bidir_lst *list, char *pattern);

char *bidir_lst_getbylen(Bidir_lst *list, long pattern);

int bidir_lst_length(Bidir_lst *list);

int read_to_list(FILE *filename, Bidir_lst *list, long *gl_str_num);

int write_from_list(FILE *filename, Bidir_lst *list);

char *list_to_string(Bidir_lst *list);

#endif /* REDUCED_LISTS_H */

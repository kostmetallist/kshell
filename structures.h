#ifndef __STRUCTURES_H__
#define __STRUCTURES_H__

struct bidir_item_lst
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

} Bidir_lst;


typedef struct
{
        int main_argc;
        int history_depth;
        int shell_pid;
	int shell_gpid;
        int jobs_num;
	int job_container_index;
        int last_fgprocess_status;
        long gen_history_numb;
        char **main_argv;

} G_data;


typedef struct
{ 
        char **containts;
        int arg_number;
        char *input_file, *output_file;
        int output_type;

} programme;


typedef struct
{
        int background;
        int prog_number;
        programme **programz;

} job;


typedef struct proc_el
{
	int number;		/* Order number */
	int ground;		/* 0 - fg, 1 - bg */
	int status;		/* 1 - running, 2 - suspended */
	int ppid;
	char *body;
	struct proc_el *prev;
	struct proc_el *next;

} proc_element;


typedef struct
{
	proc_element *head;
	proc_element *tail;

} Biproc_lst;

#endif /* STRUCTURES_H */

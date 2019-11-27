#ifndef __SERVICE_FUNC_H__
#define __SERVICE_FUNC_H__


long cmd_to_list(const char *fn, Bidir_lst *list);

long digit_number(long to_estimate);

char *int_to_ascii(int value);

int ascii_to_int(char *src, int *to_write);

char *get_envar(char *request, int *need_free, G_data *main_data);

void job_memory_erase(job **job_array);

char *str_concat(const int mode, char *the_1, char *the_2);

char *str_replace(char *general, char *outcome, char *income, long len_1, long len_2, long *point);

void history_append(Bidir_lst *history_list, char *cmd_line, long line_numb, int history_depth);

/*char *history_get(Bidir_lst *history_list, long extract_line, long general_line_numb,
		int history_depth);*/

char *history_insert(Bidir_lst *history_list, char *src, int history_depth, int *modify);

char *history_substitution(Bidir_lst *HistList, char *raw, int hist_depth);

void post_wait(int status, Biproc_lst *ProcList, char *element, G_data *metadata,
		int ground, int pid, int number);

void error_pointer(char *err_string, int position);

#endif /* SERVICE_FUNC */

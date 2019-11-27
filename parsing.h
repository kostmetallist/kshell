#ifndef __PARSING_H__
#define __PARSING_H__

/*typedef struct
{
	char *name;
	char **argz;
	int arg_number;
	char *input_file, *output_file;
	int output_type;
} programme;


typedef struct
{
	int background;
	int prog_number;
	programme **programz;
} job;*/



char *str_read_stdin(FILE *fn, int *err_code, int *ch_numb);

int list_leng_check(Bidir_lst *list, long pattern);

int list_leng_is_between(Bidir_lst *list, long index);

char *str_copy_range(char *source, int begin, int end);

void str_divide_by(char *source, char separator, int is_double, Bidir_lst *q_pos_list,
			Bidir_lst *output_list);

char *str_multinput();

char char_display(char ch_after_backslash, int quota_mode);

char *str_spaceout(char *string_link, Bidir_lst *q_pos_list);

char *str_ezspace(char *chopper);

int is_char_ended(char *source, char suspect);

int redir_determ(char *source, Bidir_lst *redir_list);

char *str_bkslash_modify(char *from);

void bkslash_list(Bidir_lst *list);

void str_mainpart_split(char *programme_str, Bidir_lst *DivBySpace);

int first_entry_index(char *src, char element, int begin_from);

char *var_substitution(char *src, G_data *metadata);

void substitution_list(Bidir_lst *list, G_data *metadata);

char *str_quota_clear(char *src);

job **parse_cmd_line(char *input, G_data *metadata, Bidir_lst *history_list);

#endif /* PARSING_H */

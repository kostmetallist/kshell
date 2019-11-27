#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "structures.h"
#include "reduced_lists.h"
#include "parsing.h"
#include "proc_monitoring.h"
#include "service_func.h"

#define  CHAR_SIZE    sizeof(char)
#define  ERR_OCCUR    perror("\n\x1b[31;1mAn error occured\x1b[37m")


long cmd_to_list(const char *fn, Bidir_lst *list)
{
	/* Returns number of commands or -1 in extraordinary case */

	long cmd_num = 0;
	FILE *datafile;

	if ((datafile = fopen(fn, "r")) == NULL)
	{
		ERR_OCCUR;
		return -1;
	}

	read_to_list(datafile, list, &cmd_num);

	if (fclose(datafile) == EOF)
	{
		ERR_OCCUR;
		return -1;
	}

	return cmd_num;
}


long digit_number(long to_estimate)
{
	long divisor = 10;
	long result = 1;

	while (to_estimate / divisor)
	{
		divisor *= 10;
		result++;
	}

	return result;
}


char *int_to_ascii(int value)
{
	/* Just converts some integer value into string that returned. */

	int quotient_operator  = 1;
	int string_length = 0;
	int offset = 0;
	char *output;

	if (!value)
	{
		output = (char *) malloc(CHAR_SIZE * 2);
		output[0] = '0';
		output[1] = '\0';
		return output;
	}

	while (value / quotient_operator)	/* Counts number of digits */
	{
		string_length++;
		quotient_operator *= 10;
	}

	output = (char *) malloc(CHAR_SIZE * (string_length + 1));
	quotient_operator /= 10;

	while (string_length-- > 1)
	{
		output[offset++] = '0' + (char) ((value / quotient_operator) % 10);	
		quotient_operator /= 10;
	}

	output[offset++] = '0' + (char) (value % 10);
	output[offset] = '\0';
	return output;
}


int ascii_to_int(char *src, int *to_write)
{
	long strtol_res;
	char *endptr;

	strtol_res = strtol(src, &endptr, 10);

	if (endptr[0] != '\0')
	{
		fprintf(stderr, "Error: Invalid integer representation.\n");
		return 1;
	}

	*to_write = strtol_res;
	return 0;
}


char *get_envar(char *request, int *need_free, G_data *main_data)
{
	/* Returns appropriate variable in response to its name.
	 * Firstly, searching is performed among service variables.
	 * If haven't found, tryin' to find among external variables
	 * that were initilized before the running of current shell.
	 * If we can't recognize name at all, just returns the same string
	 * as <request>.
	 */
	
	char *external_var;

	*need_free = 1;

	if ((strlen(request) == 1 && request[0] >= '0' && request[0] <= '9')
	||  !strcmp(request, "SHELL"))
	{
		int numerical = request[0] - '0';

		if (!strcmp(request, "SHELL"))
			numerical = 0;

		if (numerical > main_data->main_argc - 1)
		{
			fprintf(stderr, "Argument index out of range.\n");
			return NULL;
		}

		return str_copy_range
		(main_data->main_argv[numerical], 0, strlen(main_data->main_argv[numerical])+1);
	}

	if (strlen(request) == 1 && request[0] == '#')
		return int_to_ascii(main_data->main_argc);

	if (strlen(request) == 1 && request[0] == '?')
		return int_to_ascii(main_data->last_fgprocess_status);
	
	if (!strcmp(request, "PWD"))
		return getcwd(0, 0);

	if (!strcmp(request, "UID")
	||  !strcmp(request, "HOME")
	||  !strcmp(request, "USER"))
	{
		struct passwd *userpwd = NULL;
		char *response;
		uid_t user_id = getuid();
		userpwd = getpwuid(user_id);

		if (!strcmp(request, "UID"))
			return int_to_ascii(user_id);

		if (!strcmp(request, "HOME"))
		{
			response = userpwd -> pw_dir;	
			return str_copy_range(response, 0, strlen(response)+1);
		}

		if (!strcmp(request, "USER"))
		{
			response = userpwd -> pw_name;
			return str_copy_range(response, 0, strlen(response)+1);	
		}
	}

	if (!strcmp(request, "PID"))
		return int_to_ascii(main_data->shell_pid);

	*need_free = 0;

	if ((external_var = getenv(request)) != NULL )
		return external_var;

	else
		return request;
}


void job_memory_erase(job **job_array)
{	
	int JIndex = 0;
	job *JPointer;

	while ((JPointer = job_array[JIndex++]) != NULL)
	{
		int PrIndex = 0;
		programme *PrPointer;

		while (PrIndex < (JPointer->prog_number))
		{
			int StrIndex = 0;
			char *StrPointer;

			PrPointer = JPointer->programz[PrIndex];

			while ((StrPointer = PrPointer->containts[StrIndex++]) != NULL)
			{
				free(StrPointer);
			}

			free(PrPointer->containts);
			free(PrPointer->input_file);
			free(PrPointer->output_file);
			free(PrPointer);
			/*free(&PrPointer);*/
			PrIndex++;
		}

		free(JPointer->programz);
		free(JPointer);
		/*free(&JPointer);*/
	}

	free(job_array);
}


char *str_concat(const int mode, char *the_1, char *the_2)
{
	/* mode == 0 => concatenate second string to the beginning of the first;
	 * mode == 1 => ---||--- to the end
	 */

	long new_len;
	long offset;
	char *new_str;
	char elem;

	new_len = strlen(the_1) + strlen(the_2);
	new_str = (char *) malloc(CHAR_SIZE * (new_len + 1));

	if (!mode)
	{
		long offset_2 = 0;

		offset = 0;
		elem = the_2[offset];

		while (elem != '\0')
		{
			new_str[offset_2] = elem;
			offset++;
			offset_2++;
			elem = the_2[offset];
		}

		offset = 0;
		elem = the_1[offset];

		while (elem != '\0')
		{
			new_str[offset_2] = elem;
			offset++;
			offset_2++;
			elem = the_1[offset];
		}

		new_str[offset_2] = '\0';
		return new_str;
	}

	if (mode == 1)
	{
		long offset_2 = 0;

		offset = 0;
		elem = the_1[offset];

		while (elem != '\0')
		{
			new_str[offset] = elem;
			offset++;
			elem = the_1[offset];
		}

		offset_2 = offset;
		offset = 0;
		elem = the_2[offset];

		while (elem != '\0')
		{
			new_str[offset_2] = elem;
			offset++;
			offset_2++;
			elem = the_2[offset];
		}

		new_str[offset_2] = '\0';
		return new_str;
	}

	else
	{
		fprintf(stderr, "Str_concat() incorrect argument.\n");
		free(new_str);
		return NULL;
	}
}


char *str_replace(char *general, char *outcome, char *income, long len_1, long len_2, long *point)
{
	/* if either len_1 or len_2 is equal to 0, we'll calculate 
	 * lengths right there
	 */

	long difference, start_len, internal_count, out_len, in_len;
	char *dest;
	char *new_memory;
	int need_mem;

	start_len = strlen(general);

	if (len_1 || len_2)
	{
		out_len = len_1;
		in_len = len_2;
	}

	else
	{
		out_len = strlen(outcome);
		in_len = strlen(income);
	}

	need_mem = (out_len < in_len) ? 1 : 0;
	difference = need_mem ? (in_len - out_len) : (out_len - in_len);
	internal_count = 0;

	if ((dest = strstr(&general[*point], outcome)) != NULL)
	{
		long offset;
		long k, l;
		char elem;

		k = 0;
		elem = dest[0];

		while (elem != '\0')
		{
			k++;
			elem = dest[k];
		}

		new_memory = (char *) malloc(CHAR_SIZE * (start_len + 1 + difference));
		offset = start_len - k;
		k = 0;

		while (internal_count < offset)
		{
			new_memory[internal_count] = general[k];
			internal_count++;
			k++;
		}

		l = 0;

		while ((elem = income[l]) != '\0')
		{
			new_memory[internal_count] = elem;
			l++;
			internal_count++;
		}

		offset += out_len;
		*point = internal_count;

		while ((elem = general[offset]) != '\0')
		{
			new_memory[internal_count] = elem;
			offset++;
			internal_count++;
		}

		new_memory[internal_count] = '\0';
		return new_memory;
	}

	return NULL;
}


void history_append(Bidir_lst *history_list, char *cmd_line, long line_numb, int history_depth)
{
	if (line_numb != 1 && history_list -> tail -> leng >= history_depth)
	/* line_numb != 1 for avoiding case with empty list */

		bidir_lst_del_one(history_list, 1);	/* Erasing head */

	bidir_lst_append(history_list, cmd_line, line_numb);
}


char *history_insert(Bidir_lst *history_list, char *src, int history_depth, int *modify)
{
	int sign_place, begin_from, hist_number, offset, multiplier, after_digits;
	char symbol;
	char *result = str_copy_range(src, 0, strlen(src)+1);
	char *received, *before, *to_insert, *after;
	Bidir_lst SaveList;

	begin_from = 0;

	while ((sign_place = first_entry_index(result, '!', begin_from)) != -1)
	{
		begin_from = sign_place + 1;
		hist_number = 0;
		offset = 0;
		multiplier = 1;

		while ((symbol = result[begin_from + offset]) >= '0' && symbol <= '9')
			offset++;

		after_digits = begin_from + offset;

		if (!offset)	
			continue;

		while (offset--)
		{
			hist_number += (result[begin_from + offset] - '0') * multiplier;
			multiplier *= 10;
		}
		
		if ((received = bidir_lst_getbylen(history_list, hist_number)) == NULL)
		{
			fprintf(stderr, "Command with number %d wasn\'t found.\n",
				hist_number);
			free(result);
			return NULL;
		}

		*modify = 1;
		bidir_lst_init(&SaveList);
		to_insert = str_copy_range(received, 0, strlen(received)+1);
		before    = str_copy_range(result, 0, sign_place);
		after     = str_copy_range(result, after_digits, strlen(result)+1);
		
		free(result);
		bidir_lst_append(&SaveList, before, strlen(before));
		bidir_lst_append(&SaveList, to_insert, strlen(to_insert));
		bidir_lst_append(&SaveList, after, strlen(after));

		result = list_to_string(&SaveList);
		begin_from += strlen(to_insert) - 1;
		bidir_lst_erase(&SaveList);
	}
	
	return result;
}


char *history_substitution(Bidir_lst *HistList, char *raw, int hist_depth)
{
	int modify = 0;
	char *modified;

	if ((modified = history_insert(HistList, raw, hist_depth, &modify)) != NULL)
	{
		if (modify)
			printf("\n%s\n", modified);

		free(raw);
		return modified;
	}

	free(raw);
	return NULL;
}


void post_wait(int status, Biproc_lst *ProcList, char *element, G_data *metadata, int ground,
		int pid, int number)
{
	/* <number> == 0 - ordinary; else - fg. */

	if (WIFEXITED(status))
	{
		/*printf("EXITED\n");*/

		if (number)
			biproc_delbynumber(ProcList, number);

		metadata -> last_fgprocess_status = status;
	}

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
	{
		/*printf("INTERRUPTED BY ^C\n");*/

		if (number)
			biproc_delbynumber(ProcList, number);

		metadata -> last_fgprocess_status = status;
	}

	if (WIFSTOPPED(status) && 
		(WSTOPSIG(status) == SIGTSTP || WSTOPSIG(status) == SIGSTOP))
	{
		/*printf("STOPPED\n");*/
		biproc_append(ProcList, str_copy_range(element, 0, strlen(element)+1),
			metadata->job_container_index, ground, 2, pid);
		(metadata->job_container_index)++;

		if (number)
			biproc_delbynumber(ProcList, number);	
	}
}


void error_pointer(char *err_string, int position)
{
	int k = 0;

	fprintf(stderr, "%s\n", err_string);

	while (k++ < position)
		printf(" ");

	printf("^\n");
}

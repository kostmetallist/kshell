#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structures.h"
#include "reduced_lists.h"
#include "service_func.h"
#include "parsing.h"

#define  BUF_SIZE      64
#define  CTRL_SYM_NUM  7
#define  EXT_CTRL_SYM  5
#define  CHAR_SIZE     sizeof(char)
#define  DBGF          printf("[X]\n")

#define  REDIR_2MUCH   fprintf(stderr, "Aborted. Given more than 1 redirection ")


extern int sig_sigint;


char set[CTRL_SYM_NUM] = {'a', 'b', 't', 'n', 'v', 'f', 'r'};


char *str_read_stdin(FILE *fn, int *err_code, int *ch_numb)
{
	/* I can call this function universal-like, cause it can be
	 * fully used while reading some binary data. In this term, usage
	 * of <ch_numb> is essential. Particularly, in this program, we don't
	 * need to give some useful functionality to <ch_numb>.
	 */
	 
	int cch;
	int buf_count = 0;
	unsigned int mult = 1;
	unsigned long offset = 0;
	char *storage_link = NULL;
	storage_link = (char *) malloc(BUF_SIZE * CHAR_SIZE);

	if (storage_link == NULL)
	{
		fprintf(stderr, "No available memory.\n");
		*err_code = -1;
		return NULL;
	}

	storage_link[0] = '\0';

	while ((cch=fgetc(fn)) != '\n')
	{
		if (buf_count > (BUF_SIZE-1))
		{
			buf_count = 0;
			mult++;
			storage_link = (char *) realloc(storage_link, BUF_SIZE * mult * CHAR_SIZE);

			if (storage_link == NULL)
			{
				fprintf(stderr, "No available memory.\n");
				*err_code = -1;
				return NULL;
			}
		}

		if (cch == EOF)
		{	
			*err_code = -2;
			break;
		}

		storage_link[offset] = cch;
		(*ch_numb)++;
		offset++;
		buf_count++;
	}

	if (buf_count > (BUF_SIZE-1))
	{
		storage_link = (char *) realloc(storage_link,
			BUF_SIZE * mult * CHAR_SIZE + CHAR_SIZE);

		if (storage_link == NULL)
		{
			fprintf(stderr, "No available memory.\n");
			return NULL;
		}
	}

	storage_link[offset] = '\0';
	(*ch_numb)++;
	return storage_link;
}


char *str_copy_range(char *source, int begin, int end)
{	
	/* The functionality is clear. If you want to copy whole string,
	 * <begin> = 0, <end> = strlen(<source>)+1
	 */
	
	char *result;
	int offset = begin;
	int local_offset = 0;
	char sym;

	if (begin < 0 || end < begin)
	{
		fprintf(stderr, "Invalid range given to str_copy_range().\n");
		return NULL;
	}

	result = (char *) malloc(CHAR_SIZE * (end - begin + 1));

	while ((sym = source[offset++]) != '\0' && local_offset != (end - begin))
		result[local_offset++] = sym;

	result[local_offset] = '\0';
	return result;
}


void str_divide_by(char *source, char separator, int is_double, Bidir_lst *q_pos_list,
			Bidir_lst *output_list)
{
	/* As you might guess, we need to work with string <source> that is
	 * the output of str_spaceout().
	 * Use is_double == 1 if you want to divide by double entries:
	 * abcd<<efgh ==> [abcd, efgh].
	 * <output_list> must be initialized.
	 * Note, that function divides <source> by TRUE <separator> entries,
	 * means it ignores pre-backslashed and in-quotes ones.
	 */

	int offset = 0;
	int prev_border = 0;
	int backslashed = 0;
	char sym;

	while ((sym = source[offset]) != '\0')
	{
		if (sym == separator && !backslashed &&
		list_leng_is_between(q_pos_list, (long) offset) == -1)
		{
			if (is_double)
			{
				if (source[offset+1] == separator)
				{
					bidir_lst_append(output_list,
					str_copy_range(source, prev_border, offset), (long) offset);
					prev_border = offset + 2;
					offset += 2;
					continue;
				}
			}

			else
			{

				bidir_lst_append(output_list,
				str_copy_range(source, prev_border, offset), (long) offset);
				prev_border = offset + 1;
				offset++;
				continue;
			}
		}

		if (sym == '\\' && !backslashed)
		{
			backslashed = 1;
			offset++;
			continue;
		}

		backslashed = 0;
		offset++;
	}

	bidir_lst_append(output_list, str_copy_range(source, prev_border, offset), -1);
}


int is_char_ended(char *source, char suspect)
{
	/* Determines if <source> is ended by <suspect> char,
	 * regardless whitespaces at the end and <suspect> is the only entry
	 * of this char in the <source>.
	 * Keep in mind that it will ignore pre-backslashed and in-quotes entries.
	 */

	char *spaced;
	int result;
	Bidir_lst q_pos, divided;

	bidir_lst_init(&q_pos);
	bidir_lst_init(&divided);

	spaced = str_spaceout(source, &q_pos);
	str_divide_by(spaced, suspect, 0, &q_pos, &divided);	
	free(spaced);
	
	if (bidir_lst_length(&divided) == 1)	/* We haven't found any entries */
	{
		bidir_lst_erase(&q_pos);
		bidir_lst_erase(&divided);
		return -1;
	}

	if (bidir_lst_length(&divided) == 2)	/* One entry */
	{
		result = (divided.head->leng == (strlen(source)-1)) ? 0 : 1;
		bidir_lst_erase(&q_pos);
		bidir_lst_erase(&divided);
		return result;
	}

	else					/* More than one => we don't need that case */
		return 2;
}


char *str_multinput()
{
	/* Reads strings from stdin. If string is ended by '\',
	 * we continue input. At the finish, concatenating all the
	 * strings into one thing and returning.
	 */

	int str_leng;
	int err_code;
	char *entered, *result_string;
	Bidir_lst entered_list;

	bidir_lst_init(&entered_list);

	while (1)
	{
		str_leng = err_code = 0;
		entered = str_read_stdin(stdin, &err_code, &str_leng);

		if (err_code == -2)
		{
			bidir_lst_erase(&entered_list);

			if (!sig_sigint)
				return entered;

			printf("^D\n");			
			free(entered);
			return NULL;
		}

		/*if (!sig_sigint)
		{		
			bidir_lst_erase(&entered_list);
			return entered;
		}*/

		if (str_leng > 1 && entered[str_leng - 2] == '\\')
		{
			entered[str_leng - 2] = '\0';
			/* Cause we don't need backslashes at the end to be written */
			bidir_lst_append(&entered_list, entered, (long) (str_leng - 2));
			continue;
		}
		
		bidir_lst_append(&entered_list, entered, (long) (str_leng - 1));
		break;
	}

	result_string = list_to_string(&entered_list);
	bidir_lst_erase(&entered_list);
	return result_string;
}


char char_display(char ch_after_backslash, int quota_mode)
{
	/* Function must be called when found backslash in the string outside.
	 * Returns character that should be written instead of
	 * both '\' and next char OR '\0' in case of ordinary entry.
	 */
	
	if (quota_mode)
	{	
		int j = CTRL_SYM_NUM - 1;

		while (j >= 0)
		{
			if (ch_after_backslash == set[j])
				return 7 + j;		/* 7 -> look ascii codes (7 + smth) */

			j--;
		}

		if  (ch_after_backslash == '\\' || ch_after_backslash == '\'' ||
		ch_after_backslash == '\"' || ch_after_backslash == '\?')
			return ch_after_backslash;

		return '\0';
	}

	else
	{
		if  (ch_after_backslash == '#' || ch_after_backslash == '&' ||
		ch_after_backslash == '>' || ch_after_backslash == '<' ||
		ch_after_backslash == '|' || ch_after_backslash == ' ')
			return ch_after_backslash;

		return '\0';
	}
}


char *str_spaceout(char *string_link, Bidir_lst *q_pos_list)
{
	/* Function deletes whitespaces and determines places of quotes.
	 * All the places' indexes is written to <q_pos_list>.
	 * <q_pos_list> must be initialized. In case of binary data parsing,
	 * this function must receive a length of string as argument.
	 */

	char *new_place;
	int old_offset = 0;
	int new_offset = 0;
	int quotes_begin = 0;
	unsigned int space_mode = 0;
	unsigned int quota_mode = 0;
	unsigned int quota_num  = 0;
	unsigned int any_type = 1;
	char q_type = '\0';		/* We have to fill in this variable either '\"' or '\'' */
	char ch;

	new_place = (char *) malloc(CHAR_SIZE * strlen(string_link) + CHAR_SIZE);

	if ((string_link[0] == '\"' || string_link[0] == '\'') && (!old_offset))
	{
		char *just_null = (char *) malloc(CHAR_SIZE);

		just_null[0] = '\0';
		new_place[0] = string_link[0];
		bidir_lst_append(q_pos_list, just_null, new_offset);
		new_offset++;
		old_offset++;
		quota_mode = 1;
		quota_num++;
		q_type = string_link[0];
		any_type = 0;
	}

	if (string_link[0] == ' ' || string_link[0] == '\t')
	{
		while ((ch = string_link[old_offset]) == ' ' ||
		       (ch = string_link[old_offset]) == '\t')

			old_offset++;

		if (ch == '\'' || ch == '\"')
		{
			space_mode = 1;
			quotes_begin = 1;
		}

		else
			space_mode = 0;

			if (ch == '#')
			{
				space_mode = 1;
			}
	}


	while ((ch = string_link[old_offset]) != '\0')
	{
		if ((ch == ' ' || ch == '\t') && (quota_mode == 0))
		{
			space_mode = 1;
			old_offset++;
			continue;
		}

		else
		{
			if (ch == '\\' && string_link[old_offset + 1] != '\0')
			{
				/*char symb;

				symb = char_display(string_link[old_offset + 1], *quota_mode);*/

				if (space_mode)
				{
					new_place[new_offset] = ' ';
					new_offset++;
					space_mode = 0;
				}

				/*if (symb == '\0')
				{
					new_place[new_offset] = '\\';
					new_offset++;
					old_offset++;
				}

				else
				{
					new_place[new_offset] = symb;
					new_offset++;
					old_offset += 2;
				}*/

				new_place[new_offset] = ch;	
				new_place[new_offset + 1] = string_link[old_offset + 1];
				new_offset += 2;
				old_offset += 2;
				continue;
			}

			if (ch == '#')
			{
				if (quota_mode)
				{
					new_place[new_offset] = '#';
					new_offset++;
					old_offset++;
					continue;
				}

				else
				{
					if (old_offset == 0 || space_mode == 1)
					{
						/*free(string_link);*/
						new_place[new_offset] = '\0';
						return new_place;
					}
				}
			}

			if (((any_type && (ch == '\'' || ch == '\"')) || ch == q_type) &&
			(!new_offset || new_place[new_offset - 1] != '\\'))
			{	
				quota_num++;

				if (quota_num & 1)
				{
					if (!space_mode && old_offset)
					{
						error_pointer(string_link, old_offset - 1);
						fprintf(stderr, 
						"Error. Invalid symbol position before quotes.\n");
						/*free(string_link);*/		
						free(new_place);
						bidir_lst_erase(q_pos_list);
						return NULL;
					}

					if (!quotes_begin)
					{
						new_place[new_offset] = ' ';
						new_offset++;
					}

					quotes_begin = 0;
				}

				else
				{
					if (string_link[old_offset+1] != ' ' &&
					string_link[old_offset+1] != '\t' &&
					string_link[old_offset+1] != '\0')
					/* maybe also check [+1] == '\\' && [+2] == '\0'
					(for multinput) */
					{
						error_pointer(string_link, old_offset + 1);
						fprintf(stderr, 
						"Error. Invalid symbol position after quotes.\n");
						/*free(string_link);*/
						bidir_lst_erase(q_pos_list);
						free(new_place);	
						return NULL;
					}
				}

				quota_mode ^= 1;
				any_type ^= 1;
				q_type = ch;
				new_place[new_offset] = ch;

				if (1)
				{
					char *just_null = (char *) malloc(CHAR_SIZE);

					just_null[0] = '\0';
					bidir_lst_append(q_pos_list, just_null, new_offset);
				}

				space_mode = 0;
				new_offset++;
				old_offset++;
				continue;
			}

			if (space_mode)
			{
				space_mode = 0;
				new_place[new_offset] = ' ';
				new_offset++;
			}

			new_place[new_offset] = string_link[old_offset];
			new_offset++;
		}

		old_offset++;
	}

	if (quota_num & 1)
	{
		fprintf(stderr, "Error. Quotes dissonance. Input is aborted.\n");
		bidir_lst_erase(q_pos_list);
		free(new_place);
		return NULL;
	}

	new_place[new_offset] = '\0';
	return new_place;
}


char *str_ezspace(char *chopper)
{
	/* Function may be called in case when we don't need any quotes control.
	 * This one just returning <chopper> without whitespaces outside quotes.
	 */
	
	char *result;
	Bidir_lst delete_me;

	bidir_lst_init(&delete_me);
	result = str_spaceout(chopper, &delete_me);
	bidir_lst_erase(&delete_me);

	return result;
}


int list_leng_check(Bidir_lst *list, long pattern)
{
	/* Checks if <pattern> is among the list's (.leng)-elements.
	 * Returns number of list's element on success,
	 * or -1 if search was failed.
	 */

	struct bidir_item_lst *tmp = list -> head;
	int index = 1;

	while (tmp != NULL)
	{
		if (tmp -> leng == pattern)
		{
			return index;
		}

		tmp = tmp -> next;
		index++;
	}

	return -1;
}


int list_leng_is_between(Bidir_lst *list, long index)
{
	/* Checks if <index> is placed between pairs of
	 * the list's (.leng)-elements. List must be sorted by increase
	 * by its (.leng)-elements and all the elements must be non-negative.
	 * Also, list must have only even number of items.
	 */

	struct bidir_item_lst *tmp = list -> head;
	long prev, curr;

	if (list == NULL)
	{
		fprintf(stderr, "List was empty while checking between.\n");
		return -1;
	}

	while (tmp != NULL)
	{
		prev = tmp -> leng;
		curr = tmp -> next -> leng;

		if (index < curr)
		{
			if (index > prev)
				return 1;

			return -1;
		}

		tmp = tmp -> next -> next;
	}

	return -1;
}


int redir_determ(char *source, Bidir_lst *redir_list)
{
	/* Defining <source>'s strings separated by '>', ">>" and '<'.
	 * <redir_list> must be initialized. Returns 1 on success
	 * or just 0 in case of some error.
	 */

	char *normalized, *input_place, *output_place;
	unsigned int ret_value = 1;
	int placed = 0;
	Bidir_lst q_pos, div_by_input, div_by_doubleout, div_by_singleout;

	bidir_lst_init(&q_pos);
	bidir_lst_init(&div_by_input);
	bidir_lst_init(&div_by_doubleout);
	bidir_lst_init(&div_by_singleout);

	input_place = output_place = NULL;

	normalized = str_spaceout(source, &q_pos);
	str_divide_by(normalized, '<', 0, &q_pos, &div_by_input);
	bidir_lst_erase(&q_pos);

	switch (bidir_lst_length(&div_by_input))
	{
		char *another0;

		case 1:		/* Any '<' */
			another0 = str_spaceout(div_by_input.head->body, &q_pos);
			str_divide_by(another0, '>', 1, &q_pos, &div_by_doubleout);
			free(another0);
			bidir_lst_erase(&q_pos);

			switch (bidir_lst_length(&div_by_doubleout))
			{
				char *another1;

				case 1:		/* Any ">>" */
					another1 = str_spaceout(div_by_doubleout.head->body,
					&q_pos);
					str_divide_by(another1, '>', 0, &q_pos, &div_by_singleout);
					free(another1);
					bidir_lst_erase(&q_pos);

					switch (bidir_lst_length(&div_by_singleout))
					{
						case 1:		/* Any '>' */
							break;

						case 2:		/* One '>' */
							output_place = str_ezspace
							(div_by_singleout.head -> next -> body);
							placed = 1;
							break;

						default:	/* More than 1 */
							REDIR_2MUCH;
							fprintf(stderr, "(>).\n");
							ret_value = 0;
							break;
					}

					break;

				case 2:		/* One ">>" */
					another1 = str_spaceout(div_by_doubleout.head->body,
					&q_pos);
					str_divide_by(another1, '>', 0, &q_pos, &div_by_singleout);
					free(another1);
					bidir_lst_erase(&q_pos);

					if (bidir_lst_length(&div_by_singleout) != 1)
					{
						REDIR_2MUCH;
						fprintf(stderr, "(> and >>).\n");
						ret_value = 0;
						break;
					}

					bidir_lst_erase(&div_by_singleout);
					another1 = str_spaceout(div_by_doubleout.head->next->body,
					&q_pos);
					str_divide_by(another1, '>', 0, &q_pos, &div_by_singleout);
					free(another1);
					bidir_lst_erase(&q_pos);

					if (bidir_lst_length(&div_by_singleout) != 1)
					{
						REDIR_2MUCH;
						fprintf(stderr, "(>> and >).\n");
						ret_value = 0;
						break;
					}

					output_place = str_ezspace
					(div_by_doubleout.head -> next -> body);
					placed = 2;
					break;

				default:
					REDIR_2MUCH;
					fprintf(stderr, "(>>).\n");
					ret_value = 0;
					break;
			}

			break;

		case 2:		/* One '<' */
			another0 = str_spaceout(div_by_input.head->body, &q_pos);
			str_divide_by(another0, '>', 1, &q_pos, &div_by_doubleout);
			free(another0);
			bidir_lst_erase(&q_pos);

			switch (bidir_lst_length(&div_by_doubleout))
			{
				char *another2;

				case 1:		/* Any ">>" */
					another2 = str_spaceout(div_by_doubleout.head->body,
					&q_pos);
					str_divide_by(another2, '>', 0, &q_pos, &div_by_singleout);
					free(another2);
					bidir_lst_erase(&q_pos);

					switch (bidir_lst_length(&div_by_singleout))
					{
						case 1:		/* Any '>' */	
							break;

						case 2:		/* One '>' */
							output_place = str_ezspace
							(div_by_singleout.head->next->body);
							placed = 1;	
							break;

						default:	/* More than 1 */
							REDIR_2MUCH;
							fprintf(stderr, "(>).\n");
							ret_value = 0;
							break;
					}

					break;

				case 2:		/* One ">>" */
					another2 = str_spaceout(div_by_doubleout.head->body,
					&q_pos);
					str_divide_by(another2, '>', 0, &q_pos, &div_by_singleout);
					free(another2);
					bidir_lst_erase(&q_pos);

					if (bidir_lst_length(&div_by_singleout) != 1)
					{
						REDIR_2MUCH;
						fprintf(stderr, "(> and >>).\n");
						ret_value = 0;
						break;
					}

					bidir_lst_erase(&div_by_singleout);
					another2 = str_spaceout(div_by_doubleout.head->next->body,
					&q_pos);
					str_divide_by(another2, '>', 0, &q_pos, &div_by_singleout);
					free(another2);
					bidir_lst_erase(&q_pos);

					if (bidir_lst_length(&div_by_singleout) != 1)
					{
						REDIR_2MUCH;
						fprintf(stderr, "(>> and >).\n");
						ret_value = 0;
						break;
					}

					output_place = str_ezspace(div_by_doubleout.head->next->body);
					placed = 2;	
					break;

				default:
					REDIR_2MUCH;
					fprintf(stderr, "(>>).\n");
					ret_value = 0;
					break;
			}

			if (!ret_value)
				break;

			bidir_lst_erase(&div_by_doubleout);
			bidir_lst_erase(&div_by_singleout);
			another0 = str_spaceout(div_by_input.head->next->body, &q_pos);
			str_divide_by(another0, '>', 1, &q_pos, &div_by_doubleout);
			bidir_lst_erase(&q_pos);

			if (placed)
			{
				if (bidir_lst_length(&div_by_doubleout) != 1)
				{
					REDIR_2MUCH;
					fprintf(stderr, "(extra >>).\n");
					free(another0);
					ret_value = 0;
					break;
				}

				str_divide_by(another0, '>', 0, &q_pos, &div_by_singleout);
				free(another0);
				bidir_lst_erase(&q_pos);

				if (bidir_lst_length(&div_by_doubleout) != 1)
				{
					REDIR_2MUCH;
					fprintf(stderr, "(extra >).\n");
					ret_value = 0;
					break;
				}

				input_place = str_ezspace
				(div_by_input.head->next->body);
				break;
			}

			free(another0);

			switch (bidir_lst_length(&div_by_doubleout))
			{
				char *another2;

				case 1:		/* Any ">>" */
					another2 = str_spaceout(div_by_doubleout.head->body,
					&q_pos);
					str_divide_by(another2, '>', 0, &q_pos, &div_by_singleout);
					free(another2);
					bidir_lst_erase(&q_pos);

					switch (bidir_lst_length(&div_by_singleout))
					{
						case 1:		/* Any '>' */
							input_place = str_ezspace
							(div_by_input.head->next->body);
							break;

						case 2:		/* One '>' */
							input_place = str_ezspace
							(div_by_singleout.head->body);
							output_place = str_ezspace
							(div_by_singleout.head->next->body);
							placed = 1;
							break;

						default:	/* More than 1 */
							REDIR_2MUCH;
							fprintf(stderr, "(>).\n");
							ret_value = 0;
							break;
					}

					break;

				case 2:		/* One ">>" */
					another2 = str_spaceout(div_by_doubleout.head->body,
					&q_pos);
					str_divide_by(another2, '>', 0, &q_pos, &div_by_singleout);
					free(another2);
					bidir_lst_erase(&q_pos);

					if (bidir_lst_length(&div_by_singleout) != 1)
					{
						REDIR_2MUCH;
						fprintf(stderr, "(> and >>).\n");
						ret_value = 0;
						break;
					}

					bidir_lst_erase(&div_by_singleout);
					another2 = str_spaceout(div_by_doubleout.head->next->body,
					&q_pos);
					str_divide_by(another2, '>', 0, &q_pos, &div_by_singleout);
					free(another2);
					bidir_lst_erase(&q_pos);

					if (bidir_lst_length(&div_by_singleout) != 1)
					{
						REDIR_2MUCH;
						fprintf(stderr, "(>> and >).\n");
						ret_value = 0;
						break;
					}
	
					input_place = str_ezspace
					(div_by_doubleout.head->body);	
					output_place = str_ezspace
					(div_by_doubleout.head->next->body);
					placed = 2;
					break;

				default:
					REDIR_2MUCH;
					fprintf(stderr, "(>>).\n");
					ret_value = 0;
					break;
			}

			break;

		default:	/* More than 1, cause 0 can't be. */
			REDIR_2MUCH;
			fprintf(stderr, "(<).\n");
			ret_value = 0;
			break;
	}

	if (input_place != NULL)
		bidir_lst_append(redir_list, input_place, 0);

	if (output_place != NULL)
		bidir_lst_append(redir_list, output_place, (long) placed);

	free(normalized);
	bidir_lst_erase(&q_pos);
	bidir_lst_erase(&div_by_input);
	bidir_lst_erase(&div_by_doubleout);
	bidir_lst_erase(&div_by_singleout);
	return ret_value;
}


char *str_bkslash_modify(char *from)
{
	long sym_num = 0;
	long new_offset = 0;
	int  quota_mode = 0;
	char chh = from[sym_num];
	char *new = NULL;

	if (from[0] == '\'' || from[0] == '\"')
		quota_mode = 1;

	while (chh != '\0')
	{
		sym_num++;
		chh = from[sym_num];
	}
	
	new = (char *) malloc(CHAR_SIZE * (sym_num + 1));
	sym_num = 0;

	while ((chh = from[sym_num]) != '\0')
	{
		if (chh == '\\' && from[sym_num + 1] != '\0')
		{
			char chh_nxt = from[sym_num + 1];
			char to_write;
			
			to_write = char_display(chh_nxt, quota_mode);

			if (to_write != '\0')
			{
				new[new_offset] = to_write;
				new_offset++;
				sym_num += 2;
				continue;
			}

			new[new_offset] = '\\';
			new[new_offset + 1] = chh_nxt;
			new_offset += 2;
			sym_num += 2;
			continue;
		}

		new[new_offset] = chh;
		new_offset++;
		sym_num++;
	}

	new[new_offset] = '\0';
	return new;
}

void bkslash_list(Bidir_lst *list)
{
	struct bidir_item_lst *tmp = list -> head;
	char *modified;

	while (tmp != NULL)
	{
		if (tmp->body[0] != '\'')
		{
			modified = str_bkslash_modify(tmp -> body);
			free(tmp -> body);
			tmp -> body = modified;
		}
		/* ^^^ Because we need to display only in 
		 * double quotes.
		 */

		tmp = tmp -> next;
	}
}

void str_mainpart_split(char *programme_str, Bidir_lst *DivBySpace)
{
	/* Handles <programme_str>: divides by spaces until the first '>' or '<' entry.
	 * <DivBySpace> must be initialized.
	 */

	Bidir_lst QPos, DivByL, DivByR;
	char *spaced, *mainpart;

	bidir_lst_init(&QPos);
	bidir_lst_init(&DivByL);
	bidir_lst_init(&DivByR);

	spaced = str_spaceout(programme_str, &QPos);
	str_divide_by(spaced, '>', 0, &QPos, &DivByR);
	str_divide_by(spaced, '<', 0, &QPos, &DivByL);
	
	if (DivByR.head->leng == -1)
		mainpart = DivByL.head->body;

	else
	{
		if (DivByL.head->leng == -1)
			mainpart = DivByR.head->body;

		else
			mainpart = (DivByR.head->leng < DivByL.head->leng) ?
			DivByR.head->body : DivByL.head->body;
	}

	str_divide_by(mainpart, ' ', 0, &QPos, DivBySpace);
	/*bidir_lst_readall(DivBySpace);*/

	if (!strcmp(DivBySpace->tail->body, "\0"))
		bidir_lst_del_one(DivBySpace, -1);

	free(spaced);
	bidir_lst_erase(&QPos);
	bidir_lst_erase(&DivByL);
	bidir_lst_erase(&DivByR);
}


int first_entry_index(char *src, char element, int begin_from)
{
	/* Returns index of the first entry of <element>
	 * in the <src>. If not found, returns -1.
	 */
	
	int offset = begin_from;

	while (src[offset] != '\0')
	{		
		if (src[offset] == element)
			return offset;

		offset++;
	}

	return -1;
}


char *var_substitution(char *src, G_data *metadata)
{
	/* Returns modified <src>: w/ replaced var name constructions
	 * with their values.
	 */

	int entryL, entryR, begin_from;	
	char *result = str_copy_range(src, 0, strlen(src)+1);

	if (src[0] == '\'')		/* We don't need to insert within single quotes */
		return result;

	begin_from = 0;
	
	while ((entryL = first_entry_index(result, '$', begin_from)) != -1)
	{
		begin_from = entryL + 1;

		switch (result[entryL+1])	/* What's the next? */
		{
			int need_free, local_offset;
			char elem;
			char *request, *before, *after, *received, *additional;
			Bidir_lst list;	

			case '{':
				local_offset = 2;

				while ((elem = result[entryL + local_offset]) != '}'
				&& elem != '\0')
					local_offset++;

				if (elem == '\0')	/* Haven't found right brace */
				{
					begin_from = strlen(result);
					break;
				}

				entryR = entryL + local_offset;
				request = str_copy_range(result, entryL+2, entryR);
				before  = str_copy_range(result, 0, entryL);
				after   = str_copy_range(result, entryR+1, strlen(result)+1);

				bidir_lst_init(&list);
				free(result);
				bidir_lst_append(&list, before, strlen(before));
				received = get_envar(request, &need_free, metadata);

				if (!need_free)
				{
					additional = str_copy_range(received, 0, strlen(received)+1);
					bidir_lst_append(&list, additional, strlen(additional));
				}

				else
				{
					bidir_lst_append(&list, received, strlen(received));
				}

				bidir_lst_append(&list, after, strlen(after));
				result = list_to_string(&list);
				begin_from += strlen(received) - 1;
				free(request);
				bidir_lst_erase(&list);

				break;

			default:
				if ((result[entryL+1] >= '0' && result[entryL+1] <= '9')
				||  result[entryL+1] == '#' || result[entryL+1] == '?')
				{
					request = str_copy_range(result, entryL+1, entryL+2);
					before  = str_copy_range(result, 0, entryL);
					after   = str_copy_range(result, entryL+2, strlen(result)+1);

					bidir_lst_init(&list);
					free(result);
					bidir_lst_append(&list, before, strlen(before));
					received = get_envar(request, &need_free, metadata);

					if (received == NULL)	/* Only one case we have: look
						get_envar() */
						received = str_copy_range("$?", 0, 3);

					bidir_lst_append(&list, received, strlen(received));
					bidir_lst_append(&list, after, strlen(after));

					result = list_to_string(&list);
					begin_from += strlen(received);
					free(request);
					bidir_lst_erase(&list);
				}

				break;
		}
	}

	return result;
}


void substitution_list(Bidir_lst *list, G_data *metadata)
{
	struct bidir_item_lst *tmp = list -> head;
	char *to_obtain = NULL;

	while (tmp != NULL)
	{
		to_obtain = var_substitution(tmp -> body, metadata);
		free(tmp -> body);
		tmp -> body = to_obtain;
		tmp = tmp -> next;
	}
}


char *str_quota_clear(char *src)
{
	/* Returns modified <src>: deletes quotes (\' and \")
	 * at the beginning and ending of <src>.
	 * If <src> w/o quotes, just returns the same.
	 */

	char *output;

	if (src[0] == '\'' || src[0] == '\"')
		output = str_copy_range(src, 1, strlen(src) - 1);	/* Reduces 0's and last's */

	else
		output = str_copy_range(src, 0, strlen(src) + 1);	/* Copies whole string */

	return output;
}


job **parse_cmd_line(char *input, G_data *metadata, Bidir_lst *history_list)
{
	/* QUINTESSENTION */

	int job_index = 0;
	int delete_status = 0;
	char *spaced;
	Bidir_lst q_pos;
	job **jobz;	

	if (input != NULL)
	{
		if (!strcmp(input, ""))
		{
			free(input);	
			return NULL;
		}

		metadata->gen_history_numb++;
		history_append(history_list, input, metadata->gen_history_numb,
			metadata->history_depth);

		bidir_lst_init(&q_pos);
		spaced = str_spaceout(input, &q_pos);

		if (spaced == NULL || !strcmp(spaced, ""))
		{
			free(spaced);
			return NULL;
		}

		if (spaced != NULL)
		{
			Bidir_lst JobList;
			struct bidir_item_lst *ListItem;

			bidir_lst_init(&JobList);

			str_divide_by(spaced, ';', 0, &q_pos, &JobList);

			if (bidir_lst_search(&JobList, "") > 0)
			{
				fprintf(stderr, "Invalid syntax: Empty job\n");
				free(spaced);
				bidir_lst_erase(&q_pos);
				bidir_lst_erase(&JobList);
				return NULL;
			}

			metadata->jobs_num += bidir_lst_length(&JobList);
			jobz = (job **) malloc(sizeof(job *) * (metadata->jobs_num + 1));
			/* +1 for Null at the end */
			ListItem = JobList.head;
	
			while (ListItem != NULL)
			{
				char *SpacedStr;
				int amp_entry;
				Bidir_lst QPosList, ProgList;
				job *current_job;

				current_job = (job *) malloc(sizeof(job));

				bidir_lst_init(&QPosList);
				bidir_lst_init(&ProgList);

				SpacedStr = str_spaceout(ListItem->body, &QPosList);
				amp_entry = is_char_ended(SpacedStr, '&');

				if (amp_entry == 1 || amp_entry == 2)
				{
					fprintf(stderr, "Invalid ampersand position.\n");
					delete_status = 1;
				}

				/* if 0, it was; if -1, it wasn't */

				else
				{
					if (!amp_entry)
						SpacedStr[strlen(SpacedStr) - 1] = '\0';
				}

				/* ^^^ Avoiding ampersand at the end of string because
				 * we don't need it as argument.
				 */

				if (1)	/* Just another block of operators. Just if (1). */
				{
					Bidir_lst RedirList, SpaceList;
					struct bidir_item_lst *lll;
					programme **progz;
					int prog_index = 0;

					current_job -> background = (!amp_entry) ? 1 : 0;
					bidir_lst_init(&RedirList);
					bidir_lst_init(&SpaceList);
					str_divide_by(SpacedStr, '|', 0, &QPosList, &ProgList);

					current_job -> prog_number = bidir_lst_length(&ProgList);
					progz = (programme **) malloc(sizeof(programme *) *
					current_job -> prog_number);

					lll = ProgList.head;

					while (lll != NULL)
					{
						programme *prog;

						prog = (programme *) malloc(sizeof(programme));

						redir_determ(lll -> body, &RedirList);
						bkslash_list(&RedirList);
						substitution_list(&RedirList, metadata);

						/* memory manage */
						if (list_leng_check(&RedirList, 0) != -1) /*inp*/
						{
							char *pre_cleared =
								str_copy_range(RedirList.head->body,
								0, strlen(RedirList.head->body)+1);

							prog -> input_file =
								str_quota_clear(pre_cleared);
							free(pre_cleared);
						}

						else
						{
							char *empty;
							empty = (char *) malloc(CHAR_SIZE);
							empty[0] = '\0';
							prog -> input_file = empty;
						}	

						if (list_leng_check(&RedirList, 1) != -1) /*out >*/
						{
							char *pre_cleared =
								str_copy_range(RedirList.tail->body,
								0, strlen(RedirList.tail->body)+1);

							prog -> output_file =
								str_quota_clear(pre_cleared);
							prog -> output_type = 1;
							free(pre_cleared);
						}

						else
						{
							if (list_leng_check
							(&RedirList, 2) != -1) /*out >>*/
							{
								char *pre_cleared =
									str_copy_range
									(RedirList.tail->body,
									0, strlen
									(RedirList.tail->body)+1);

								prog -> output_file =
									str_quota_clear(pre_cleared);
								prog -> output_type = 2;
								free(pre_cleared);
							}

							else
							{
								char *empty;
								empty = (char *) malloc(CHAR_SIZE);
								empty[0] = '\0';
								prog -> output_file = empty;
							}
						}
						/* end */
						bidir_lst_erase(&RedirList);

						str_mainpart_split(lll -> body, &SpaceList);	

						if ((bidir_lst_search(&SpaceList, "") > 0 ||
							SpaceList.head == NULL) && !delete_status)
						{
							fprintf(stderr, "Invalid syntax: Empty program.\n");
							delete_status = 1;
						}

						bkslash_list(&SpaceList);
						substitution_list(&SpaceList, metadata);

						/* memory manage */	

						if (1)
						{
							char **content;
							struct bidir_item_lst *piece =
							SpaceList.head;
							int arg_index = 0;

							content = (char **) malloc(sizeof(char *) *
							(bidir_lst_length(&SpaceList) + 1));
							/* +1 for Null at the end */

							while (piece)
							{
								char *pre_cleared =
									str_copy_range(piece->body,
									0, strlen(piece->body)+1);

								content[arg_index++] =
									str_quota_clear(pre_cleared);
								piece = piece -> next;
								free(pre_cleared);
							}

							content[arg_index] = NULL;
							prog -> containts = content;
							prog -> arg_number = arg_index - 1;
						}
						/* end */
						progz[prog_index++] = prog;
						bidir_lst_erase(&SpaceList);
						lll = lll -> next;
					}

					current_job -> programz = progz;
					bidir_lst_erase(&ProgList);
					bidir_lst_erase(&QPosList);
				}

				free(SpacedStr);
				jobz[job_index++] = current_job;	
				ListItem = ListItem -> next;
			}

			jobz[job_index] = NULL;
			bidir_lst_erase(&JobList);
			free(spaced);
		}

		else	/* spaced == NULL */
			jobz = NULL;

		bidir_lst_erase(&q_pos);
	}

	if (delete_status)
	{
		job_memory_erase(jobz);
		return NULL;
	}

	return jobz;
}

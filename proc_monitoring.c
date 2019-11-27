#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "structures.h"
#include "proc_monitoring.h"


void biproc_init(Biproc_lst *listname)
{
	listname -> head = NULL;
	listname -> tail = NULL;
}


void biproc_append(Biproc_lst *listname, char *data, int number, int ground, int status, int pid)	
{
	proc_element *new_item;
	new_item = (proc_element *) malloc(sizeof(proc_element));

	new_item -> number = number;
	new_item -> ground = ground;
	new_item -> status = status;
	new_item -> body   = data;
	new_item -> ppid   = pid;

	if (listname -> head == NULL && listname -> tail == NULL)
	{
		new_item -> prev = NULL;
		new_item -> next = NULL;	
		listname -> head = new_item;
		listname -> tail = new_item;
		return;
	}

	new_item -> prev = listname -> tail;
	new_item -> next = NULL;
	listname -> tail -> next = new_item;
	listname -> tail = new_item;
}


void biproc_erase(Biproc_lst *listname)
{
	proc_element *tmp   = listname -> head;
	proc_element *delet = NULL;

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


proc_element *biproc_getbynumber(Biproc_lst *listname, int elem_num)
{
	/* If not found, returns NULL. */

	proc_element *tmp = listname -> head;

	while (tmp != NULL)
	{
		if (tmp->number == elem_num)
			return tmp;

		tmp = tmp -> next;
	}

	return NULL;
}


int biproc_delbynumber(Biproc_lst *listname, int elem_num)
{
	/* On success, 0 is returned; 1 if not found. */

	proc_element *victim = listname -> head;

	while (victim != NULL)
	{
		if (victim->number == elem_num)
		{
			if (victim == listname->head && victim == listname->tail)
			{
				free(victim -> body);
				free(victim);
				listname -> head = NULL;
				listname -> tail = NULL;
				return 0;
			}

			if (victim == listname->head)
			{
				victim -> next -> prev = NULL;
				listname -> head = victim -> next;
				free(victim -> body);
				free(victim);
				return 0;
			}

			if (victim == listname->tail)
			{
				victim -> prev -> next = NULL;
				listname -> tail = victim -> prev;
				free(victim -> body);
				free(victim);
				return 0;
			}

			victim -> prev -> next = victim -> next;
			victim -> next -> prev = victim -> prev;
			free(victim -> body);
			free(victim);
			return 0;
		}

		victim = victim -> next;
	}

	return 1;
}


int biproc_findbypid(Biproc_lst *listname, int pid_to_delete)
{
	proc_element *tmp = listname -> head;

	while (tmp != NULL)
	{
		if (tmp->ppid == pid_to_delete)
			return tmp->number;

		tmp = tmp -> next;
	}
	
	return -1;
}


void biproc_readall(Biproc_lst *listname)
{
	proc_element *tmp = listname -> head;	

	if (tmp == NULL)
	{
		fprintf(stderr, "Nothing to print: Empty list.\n");
		return;
	}

	printf("\n");

	while (tmp != NULL)
	{
		int offset = 0;
		unsigned char elem = tmp -> body[offset];

		printf("[%d] ", tmp -> number);
		
		switch (tmp -> ground)
		{
			case 0:
				printf("FG ");
				break;

			case 1:
				printf("BG ");
				break;

			default:
				printf("?? ");
				break;
		}

		switch (tmp -> status)
		{
			case 1:
				printf("RUNNING   ");
				break;

			case 2:
				printf("SUSPENDED ");
				break;

			default:
				printf("????????? ");
				break;
		}

		while (elem != '\0')
		{
			putchar((int) elem);
			offset++;
			elem = tmp -> body[offset];
		}

		printf(" | %d |\n", tmp -> ppid);	
		tmp = tmp -> next;
	}
}


int give_term_group(int process_group)
{
	int result;

	result = tcsetpgrp(0, process_group);

	if (result == -1)
	{
		perror("Error while setting terminal group.\n");
		return 1;
	}

	return 0;
}


void zombie_reaper(Biproc_lst *ProcList)
{
	proc_element *tmp = ProcList -> head;
	int result, possible_number, to_begin;

	to_begin = 0;

	while (tmp != NULL)
	{
		do
		{
			result = waitpid(tmp->ppid, NULL, WNOHANG | WUNTRACED);

			switch (result)
			{
				case -1:
					possible_number = biproc_findbypid(ProcList, tmp->ppid);

					if (possible_number == -1)
					{
						printf("Don\'t have such pid in list.\n");
						break;
					}

					printf("Done: %s [%d]\n", tmp->body, tmp->ppid);
					biproc_delbynumber(ProcList, possible_number);
					to_begin = 1;
					break;

				case 0:
					/*printf("No changes occured: %s\n", tmp->body);*/
					break;
			}

		} while (result > 0);

		if (to_begin)
		{
			tmp = ProcList -> head;
			to_begin = 0;
		}

		else
			tmp = tmp -> next;
	}
}

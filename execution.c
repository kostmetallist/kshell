#define _POSIX_SOURCE
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structures.h"
#include "reduced_lists.h"
#include "service_func.h"
#include "parsing.h"
#include "proc_monitoring.h"
#include "execution.h"

#define DBG        printf("[X]\n")

#define INT_CD     1
#define INT_PWD    2
#define INT_JOBS   3
#define INT_FG     4
#define INT_BG     5
#define INT_EXIT   6
#define INT_HIST   7
#define INT_MCAT   8
#define INT_MSED   9
#define INT_MGRP   10


int was_cont = 0;


void handler_all(int sig_num)
{
	fprintf(stderr, "Catched signal %d\n", sig_num);
	signal(sig_num, handler_all);
}


void handler_sigchld(int sig_num)
{
	/*fprintf(stderr, "\nSIGCHLD caught.\n");*/
	/* I KNOW THAT IT IS VERY COOL HANDLER */
	signal(SIGCHLD, handler_sigchld);
}


void ppline_cont(int sig_num)
{
	raise(SIGCONT);
	was_cont = 1;
	signal(SIGCONT, ppline_cont);
}


int embed_cd(char **data, int arg_num)
{
	if (arg_num != 1)
	{
		fprintf(stderr, "Invalid argument number: Should be 1.\n");
		return 1;
	}

	if (chdir(data[1]) == -1)
	{
		perror("Changing directory error");
		return 2;
	}

	return 0;
}


int embed_pwd(int arg_num)
{
	char *pwd;

	if (arg_num)
	{
		fprintf(stderr, "Invalid argument number: Should be 0.\n");
		return 1;
	}

	if ((pwd = getcwd(0, 0)) != NULL)
	{
		printf("%s\n", pwd);
		free(pwd);
		return 0;
	}

	else
	{
		perror("An error was occured while getting current working directory");
		free(pwd);
		return 2;
	}
}


int embed_fg(char **data, int arg_num, G_data *metadata, int ground, Biproc_lst *ProcList)
{
	int number, result;
	proc_element *piece;

	if (arg_num != 1)
	{
		fprintf(stderr, "Invalid argument number: Should be 1.\n");
		return 1;
	}

	result = ascii_to_int(data[1], &number);

	if (result)
		return 2;

	if ((piece = biproc_getbynumber(ProcList, number)) != NULL)
	{
		int status_external;

		signal(SIGINT, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);	

		if (kill(piece->ppid, SIGTSTP) == -1)
		{
			perror("Kill");
			return 3;
		}

		give_term_group(piece->ppid);	

		if (kill(piece->ppid, SIGCONT) == -1)
		{
			perror("Kill");
			return 3;
		}

		sleep(2);
	
		if (waitpid(piece->ppid, &status_external, WUNTRACED) == -1)
		{	
			perror("Waitpid inside");

 			if (waitpid(piece->ppid, &status_external, WNOHANG) == -1)
				perror("Waitpid superinside");
		}

		give_term_group(metadata->shell_gpid);

		post_wait(status_external, ProcList, piece->body, metadata, ground,
			piece->ppid, number);

		return 0;
	}

	else
	{
		fprintf(stderr, "Incorrect job number: Not found.\n");
		return 5;
	}
}


int embed_bg(char **data, int arg_num, G_data *metadata, int ground, Biproc_lst *ProcList)
{
	/* It can be done only in one case - when we're tryin' to convert stopped
	 * foreground process into running background one.
	 */

	int number, result;
	proc_element *piece;

	if (arg_num != 1)
	{
		fprintf(stderr, "Invalid argument number: Should be 1.\n");
		return 1;
	}

	result = ascii_to_int(data[1], &number);

	if (result)
		return 2;

	if ((piece = biproc_getbynumber(ProcList, number)) != NULL)
	{
		int target = piece -> ppid;

		if (piece->ground == 1)	/* BG */
			return 0;

		if (kill(target, SIGCONT) == -1)
		{
			perror("Kill");
			return 4;
		}

		was_cont = 0;	/* Global */
		piece -> ground = 1;
		piece -> status = 1;
		sleep(1);	/* For sure */
		give_term_group(metadata->shell_gpid);

		return 0;
	}

	else
	{
		fprintf(stderr, "Incorrect job number: Not found.\n");
		return 5;
	}
}


int embed_mcat(char **data, int arg_num)
{	
	Bidir_lst BufList;
	FILE *filename;
	char *entered;
	long str_numb = 0;
	int error_code, sym_numb;

	if (arg_num > 1)
	{
		fprintf(stderr, "Invalid argument number: Should be 0 or 1.\n");
		return 1;
	}

	bidir_lst_init(&BufList);

	if (arg_num == 1)
	{
		if ((filename = fopen(data[1], "r")) == NULL)
		{
			perror("File open error");
			return 2;
		}

		read_to_list(filename, &BufList, &str_numb);

		if (fclose(filename) == EOF)
		{
			perror("File close error");
			return 3;
		}

		write_from_list(stdout, &BufList);
		bidir_lst_erase(&BufList);
	}

	else
	{
		while (1)
		{
			error_code = 0;
			sym_numb = 0;

			entered = str_read_stdin(stdin, &error_code, &sym_numb);	

			if (error_code == -1)	/* Memory lack */
			{
				free(entered);
				return 4;
			}

			if (error_code == -2)	/* EOF */
			{
				printf("%s", entered);
				free(entered);
				break;
			}

			printf("%s\n", entered);
			free(entered);
		}
	}

	return 0;
}


int embed_msed(char **data, int arg_num)
{
	char *entered, *emigrant, *immigrant, *result;
	int error_code, sym_numb;

	if (arg_num != 2)
	{
		fprintf(stderr, "Invalid argument number: Should be 2.\n");
		return 1;
	}

	emigrant  = data[1];
	immigrant = data[2];

	if (!strcmp(emigrant, "^") || !strcmp(emigrant, "$"))	/* Insert mode */
	{
		int mode;

		mode = (!strcmp(emigrant, "^")) ? 0 : 1;

		while (1)
		{
			error_code = 0;
			sym_numb = 0;

			entered = str_read_stdin(stdin, &error_code, &sym_numb);	

			if (error_code == -1)	/* Memory lack */
			{
				free(entered);
				return 2;
			}

			if (error_code == -2)	/* EOF */
			{	
				free(entered);
				break;
			}

			result = str_concat(mode, entered, immigrant);

			if (result == NULL)
				return 3;
			
			printf("%s\n", result);
			free(result);
			free(entered);
		}
	}

	else		/* Replace mode */
	{
		long len_1, len_2;

		len_1 = strlen(emigrant);
		len_2 = strlen(immigrant);

		while (1)
		{
			long iterator = 0;

			error_code = 0;
			sym_numb = 0;

			entered = str_read_stdin(stdin, &error_code, &sym_numb);	

			if (error_code == -1)	/* Memory lack */
			{
				free(entered);
				return 1;
			}

			if (error_code == -2)	/* EOF */
			{	
				free(entered);
				break;
			}

			while ((result = str_replace(entered, emigrant, immigrant, len_1, len_2,
			&iterator)) != NULL)
			{
				free(entered);
				entered = result;	
			}

			printf("%s\n", entered);
			free(result);
			free(entered);
		}
	}

	return 0;
}


int embed_mgrp(char **data, int arg_num)
{
	char *entered;
	char *example;
	int error_code, sym_numb;

	switch (arg_num)
	{
		case 1:
			example = data[1];

			while (1)
			{
				error_code = 0;
				sym_numb = 0;

				entered = str_read_stdin(stdin, &error_code, &sym_numb);	

				if (error_code == -1)	/* Memory lack */
				{
					free(entered);
					return 1;
				}

				if (error_code == -2)	/* EOF */
				{	
					free(entered);
					break;
				}

				if (strstr(entered, example))
					printf("%s\n", entered);

				free(entered);
			}

			break;

		case 2:
			if (!strcmp(data[2], "-v"))
			{
				example = data[1];

				while (1)
				{
					error_code = 0;
					sym_numb = 0;

					entered = str_read_stdin(stdin, &error_code, &sym_numb);	

					if (error_code == -1)	/* Memory lack */
					{
						free(entered);
						return 2;
					}

					if (error_code == -2)	/* EOF */
					{	
						free(entered);
						break;
					}

					if (strstr(entered, example) == NULL)
						printf("%s\n", entered);

					free(entered);
				}
			}

			else
			{
				fprintf(stderr, "Invalid argument 2 notation.\n");
				return 3;
			}

			break;

		default:
			fprintf(stderr, "Invalid argument number: Should be 1 or 2.\n");
			return 4;
	}

	return 0;
}


int *stream_redir(char *inp_redir, char *out_redir, int out_type, int *changed_fds)
{
	int inp_fd, out_fd;

	if (strcmp(inp_redir, ""))	/* If redirection was */
	{
		inp_fd = open(inp_redir, O_RDONLY);

		if (inp_fd == -1)
		{
			perror("Input redirection failed");
			return changed_fds;
		}
		
		close(0);
		dup2(inp_fd, 0);
		changed_fds[0] = inp_fd;
	}

	/* O_APPEND may lead to corrupted files on NFS filesystems
	 * if more than one process appends data to a  file  at  once.
	 *   (c) man open
	 */

	if (strcmp(out_redir, ""))	/* If redirection was */
	{
		if (out_type == 1)	/* Rewrite */
		{
			out_fd = open(out_redir, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

			if (out_fd == -1)
			{
				perror("Output redirection failed");
				return changed_fds;
			}

			close(1);
			dup2(out_fd, 1);
			changed_fds[1] = out_fd;
		}

		else			/* Append */
		{
			out_fd = open(out_redir, O_WRONLY | O_CREAT | O_APPEND,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

			if (out_fd == -1)
			{
				perror("Output redirection failed");
				return changed_fds;
			}

			close(1);
			dup2(out_fd, 1);
			changed_fds[1] = out_fd;
		}
	}

	return changed_fds;
}


int cmd_separated_run(job **job_data, programme *prog, Bidir_lst *CmdList, Bidir_lst *HistList,
			Biproc_lst *ProcList, int ground, G_data *metadata)
{
	int cmd_index;

	cmd_index = bidir_lst_search(CmdList, prog->containts[0]);

	if (cmd_index != -1)    /* -- INTERNAL -- */
	{	
		switch (cmd_index)
		{
			case INT_CD:
				embed_cd(prog->containts, prog->arg_number);
				break;

			case INT_PWD:
				embed_pwd(prog->arg_number);
				break;

			case INT_JOBS:
				biproc_readall(ProcList);
				break;

			case INT_FG:
				embed_fg(prog->containts, prog->arg_number, metadata, ground,
					ProcList);
				break;

			case INT_BG:
				embed_bg(prog->containts, prog->arg_number, metadata, ground,
					ProcList);
				break;

			case INT_EXIT:	/* We just do nothing cause it's separated. */
				break;

			case INT_HIST:
				printf("History ");
				bidir_lst_readall(HistList);
				break;

			case INT_MCAT:
				embed_mcat(prog->containts, prog->arg_number);
				break;

			case INT_MSED:
				embed_msed(prog->containts, prog->arg_number);
				break;

			case INT_MGRP:
				embed_mgrp(prog->containts, prog->arg_number);
				break;
		}

		job_memory_erase(job_data);
		bidir_lst_erase(CmdList);
		bidir_lst_erase(HistList);
		biproc_erase(ProcList);
		exit(0);
		return 0;
	}

	else                    /* -- EXTERNAL -- */
	{
		execvp(prog->containts[0], prog->containts);
		perror("Exec error");
		job_memory_erase(job_data);
		bidir_lst_erase(CmdList);
		bidir_lst_erase(HistList);
		biproc_erase(ProcList);
		exit(0);
		return 0;
	}
}


int cmds_exe(job **job_begin, Bidir_lst *CmdList, Bidir_lst *HistList, Biproc_lst *ProcList,
			G_data *metadata)
{
	/* Implementing basic- and pipeline- linked command execution.
	 * Returns 1 if we need to terminate the main().
	 */
	
	int job_count = 0;
	job *working = NULL;

	while ((working = job_begin[job_count++]) != NULL)
	{
		int program_index = 0;
		int is_ppline, pipeline_pid, pipeline_status, in_ppline_stopped;
		int pipe_new[2];
		int pipe_old[2];	

		signal(SIGCHLD, handler_sigchld);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);	

		is_ppline = (working->prog_number == 1) ? 0 : 1;


		/* -- DEDICATED -- */

		if (!is_ppline)
		{
			int cmd_index;
			int saved_streams[2];
			int changed_streams[2] = {-1, -1};
			programme *prog = working->programz[0];


			/* -- BACKGROUND -- */

			if (working->background)
			{
				int pid_prog = -1;
				int pid_son; 

				switch (pid_prog = fork())
				{
					case 0:
						signal(SIGTTIN, handler_all);
						signal(SIGTTOU, handler_all);
						signal(SIGTSTP, SIG_DFL);

						pid_son = getpid();	

						if (setpgid(pid_son, pid_son) == -1)
						{
							perror("Setgpid");
							break;
						}	

						stream_redir(working->programz[program_index]->
						input_file, working->programz[program_index]->
						output_file, working->programz[program_index]->
						output_type, changed_streams);

						cmd_separated_run(job_begin, working->
						programz[program_index], CmdList, HistList, ProcList,
						working->background, metadata);	

						break;

					case -1:
						perror("Background fork");
						break;

					default:	
						biproc_append(ProcList, str_copy_range
						(working->programz[program_index]->containts[0], 0,
						strlen(working->programz[program_index]->containts[0])
						+1), metadata->job_container_index, 1, 1, pid_prog);
						(metadata->job_container_index)++;

						break;

				} /* Switch */

				continue;
			}



			/* -- FOREGROUND -- */

			cmd_index = bidir_lst_search(CmdList, prog->containts[0]);

			if (cmd_index != -1)    /* -- INTERNAL -- */
			{
				saved_streams[0] = dup(0);
				saved_streams[1] = dup(1);
				stream_redir(prog->input_file, prog->output_file, prog->output_type,
					changed_streams);

				switch (cmd_index)
				{
					case INT_CD:
						embed_cd(prog->containts, prog->arg_number);
						break;

					case INT_PWD:
						embed_pwd(prog->arg_number);
						break;

					case INT_JOBS:
						biproc_readall(ProcList);
						break;

					case INT_FG:
						embed_fg(prog->containts, prog->arg_number, metadata,
						working->background, ProcList);
						break;

					case INT_BG:
						embed_bg(prog->containts, prog->arg_number, metadata,
						working->background, ProcList);
						break;

					case INT_EXIT:
						return 1;	/* Only if we handling 1 as returning
								 * of cmds_exe() in the main */
					case INT_HIST:
						printf("History ");
						bidir_lst_readall(HistList);
						break;

					case INT_MCAT:
						embed_mcat(prog->containts, prog->arg_number);
						break;

					case INT_MSED:
						embed_msed(prog->containts, prog->arg_number);
						break;

					case INT_MGRP:
						embed_mgrp(prog->containts, prog->arg_number);
						break;
				}

				if (changed_streams[0] != -1)
				{
					close(changed_streams[0]);
					dup2(saved_streams[0], 0);
				}

				if (changed_streams[1] != -1)
				{
					close(changed_streams[1]);
					dup2(saved_streams[1], 1);
				}
			}

			else                    /* -- EXTERNAL -- */
			{
				int pid_external = -1;
				int status_external, pid_son;
				int changed_streams[2] = {-1, -1};

				signal(SIGTSTP, SIG_IGN);
				signal(SIGINT, SIG_IGN);
				signal(SIGTTOU, SIG_IGN);

				switch (pid_external = fork())
				{
					case 0:
						signal(SIGTSTP, SIG_DFL);
						signal(SIGINT, SIG_DFL);
						signal(SIGTTIN, handler_all);
						signal(SIGTTOU, handler_all);

						pid_son = getpid();	

						if (setpgid(pid_son, pid_son) == -1)
						{
							perror("Setgpid");
							break;
						}	

						stream_redir(working->programz[program_index]->
							input_file, working->programz[program_index]->
							output_file, working->programz[program_index]->
							output_type, changed_streams);
						execvp(prog->containts[0], prog->containts);
						perror("Exec error");
						job_memory_erase(job_begin);
						bidir_lst_erase(CmdList);
						bidir_lst_erase(HistList);
						biproc_erase(ProcList);
						exit(0);

					case -1:
						perror("Fork error");
						break;

					default:
						give_term_group(pid_external);

						if (waitpid(pid_external,
						&status_external, WUNTRACED) == -1)
						{
							perror("Waitpid error");
							break;	
						}	

						post_wait(status_external, ProcList,
							prog->containts[0], metadata,
							working->background, pid_external, 0);

						/*if (tcsetpgrp(0, metadata->shell_gpid) == -1)
							perror("Shell returning terminal error");*/

						give_term_group(metadata->shell_pid);

						break;
				}	/* Switch */
			}	/* External cmd */

			continue;
		}


		/* -- PIPELINE MODE -- */

		pipeline_pid = fork();

		switch (pipeline_pid)
		{
			case -1:
				perror("Pipeline fork");
				break;

			case 0:		/* -- PIPELINE PROCESS -- */
			{
				int pid_myself;		

				pid_myself = getpid();

				if (setpgid(pid_myself, pid_myself) == -1)
				{
					perror("Setgpid");
					break;
				}

				was_cont = 0;	/* Global */

				while (program_index < working->prog_number)
				{	
					int pid_prog = -1;
					int status_prog;	

					signal(SIGTSTP, SIG_IGN);
					signal(SIGINT, SIG_DFL);	/* Ignore this later XXX */
					signal(SIGTTOU, SIG_IGN);
					signal(SIGCONT, ppline_cont);

					pipe(pipe_new);
					pid_prog = fork();

					if (pid_prog == -1)
					{
						perror("Fork error");
						return 1;
					}

					if (!pid_prog)
					{
						int changed_streams[2] = {-1, -1};
						int pid_son;

						signal(SIGTSTP, SIG_DFL);
						signal(SIGINT, SIG_DFL);
						signal(SIGTTIN, handler_all);
						signal(SIGTTOU, handler_all);

						if (program_index == 0)
						{	
							dup2(pipe_new[1], 1);
							close(pipe_new[1]);
							close(pipe_new[0]);
						}

						if (program_index == (working->prog_number-1))
						{		
							dup2(pipe_old[0], 0);	
							close(pipe_old[0]);
							close(pipe_old[1]);
							close(pipe_new[0]);
							close(pipe_new[1]);
						}

						if (program_index && (program_index !=
									(working->prog_number-1)))
						{
							dup2(pipe_old[0], 0);
							dup2(pipe_new[1], 1);
							close(pipe_old[0]);
							close(pipe_old[1]);
							close(pipe_new[0]);
							close(pipe_new[1]);
						}

						stream_redir(working->programz[program_index]->
						input_file, working->programz[program_index]->
						output_file, working->programz[program_index]->
						output_type, changed_streams);

						pid_son = getpid();

						if (setpgid(pid_son, pid_son) == -1)
							perror("Setgpid");

						cmd_separated_run(job_begin, working->
						programz[program_index], CmdList, HistList,
						ProcList, working->background, metadata);

					}		/* END CHILD */

					/* FATHER (Pipeline process) */

					else
					{
						if (!working->background)
							give_term_group(pid_prog);	

						do
						{
							if (was_cont)
							{
								give_term_group(pid_prog);
								was_cont = 0;
							}

							if (waitpid(pid_prog, &status_prog, WUNTRACED)
								== -1)
							{
								/*perror("Waitpid pipeline error");*/

								if (was_cont)
								{
									give_term_group(pid_prog);
									was_cont = 0;

									if (waitpid(pid_prog,
									&status_prog, WUNTRACED)
									== -1)
									{
										perror("Waitpid");
										break;
									}
								}
							}

							in_ppline_stopped =
								(WIFSTOPPED(status_prog) &&
								WSTOPSIG(status_prog) == SIGTSTP) ?
								1 : 0;

							if (in_ppline_stopped)
							{	
								give_term_group(metadata->shell_pid);

								raise(SIGSTOP); /*Stop ppline proc*/

								if (!working->background)
									give_term_group(pid_prog);

								kill(pid_prog, SIGCONT); /*
								After continuing pipline */
							}

							if (WIFSIGNALED(status_prog)
								&& WTERMSIG(status_prog) == SIGINT)
							{	
								job_memory_erase(job_begin);
								bidir_lst_erase(CmdList);
								bidir_lst_erase(HistList);
								biproc_erase(ProcList);
								exit(0);
								kill(getpid(), SIGINT);
							}

						} while (in_ppline_stopped);

						if (program_index == 0)
						{
							close(pipe_new[1]);
						}

						if (program_index == (working->prog_number-1))
						{
							close(pipe_new[0]);	/* We just don't need fresh pipe */
							close(pipe_new[1]);	/* ______________________ */
							close(pipe_old[0]);	/* And closing pre-last reading end of pipe*/
						}

						if (program_index && (program_index !=
									(working->prog_number-1)))
						{
							close(pipe_new[1]);	/* Don't need to write there more */
							close(pipe_old[0]);
						}

						pipe_old[0] = pipe_new[0];
						pipe_old[1] = pipe_new[1];

						give_term_group(metadata->shell_gpid);
						program_index++;
					}
				} /* While programz */

				job_memory_erase(job_begin);
				bidir_lst_erase(CmdList);
				bidir_lst_erase(HistList);
				biproc_erase(ProcList);
				exit(0);
				break;

			} /* Pipeline process */

			default:	/* -- Shell -- */
			{	
				if (!working->background)
				{
					give_term_group(pipeline_pid);	

					if (waitpid(pipeline_pid,
						&pipeline_status, WUNTRACED) == -1)
					{
						perror("Waitpid error here");
						break;
					}

					post_wait(pipeline_status, ProcList, "Pipeline", metadata,
						working->background, pipeline_pid, 0);
				}

				else
				{
					biproc_append(ProcList, str_copy_range("Pipeline", 0,
						strlen("Pipeline")+1),
						metadata->job_container_index, 1, 1, pipeline_pid);
					(metadata->job_container_index)++;
				}

				give_term_group(metadata->shell_pid);
				break;
			}
		} /* Switch (pipeline pid) */

		continue;
	}

	return 0;
}

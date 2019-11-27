#define _POSIX_SOURCE
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <sys/types.h>

#include <signal.h>
#include <termios.h>
#include <pwd.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structures.h"
#include "reduced_lists.h"
#include "service_func.h"
#include "parsing.h"
#include "execution.h"
#include "proc_monitoring.h"

#define  ERR_OCCUR    perror("\n\x1b[31;1mAn error occured\x1b[37m")
#define  DBG          printf("[X]\n")


int sig_sigint = 1;


void handler_sigint(int sig_num)
{
	sig_sigint = 0;
}


int main(int argc, char **argv)
{
	Bidir_lst cmd_list, history_list;
	Biproc_lst proc_list;
	G_data meta_data;

	meta_data.shell_pid = getpid();
	meta_data.shell_gpid = getpgid(meta_data.shell_pid);
	meta_data.main_argc = argc;
	meta_data.main_argv = argv;
	meta_data.jobs_num  = 0;
	meta_data.job_container_index = 1;
	meta_data.last_fgprocess_status = 0;
	meta_data.history_depth = 8;
	meta_data.gen_history_numb = 0;	
	
	bidir_lst_init(&cmd_list);
	bidir_lst_init(&history_list);
	biproc_init(&proc_list);

	cmd_to_list("cmds.txt", &cmd_list);


	while (1)
	{
		char *cmd_line, *history_implemented;
		job **SSKO = NULL;	/* Idk why SSKO. Maybe SuperSecureKernelObject. */

		signal(SIGINT, handler_sigint);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);

		zombie_reaper(&proc_list);

		printf("\n\x1b[32;1mkshell $ \x1b[37m");
		cmd_line = str_multinput();

		if (cmd_line == NULL)		/* I.e. if ^D */
			break;

		if (!sig_sigint)
		{
			sig_sigint = 1;
			free(cmd_line);
			continue;
		}

		if ((history_implemented = history_substitution(&history_list, cmd_line,
			meta_data.history_depth)) == NULL)
			continue;

		SSKO = parse_cmd_line(history_implemented, &meta_data, &history_list);

		if (SSKO == NULL)	/* For avoiding empty and comment-only strings */
			continue;

		if (cmds_exe(SSKO, &cmd_list, &history_list, &proc_list, &meta_data)) /* For exit */
		{
			job_memory_erase(SSKO);
			bidir_lst_erase(&history_list);
			bidir_lst_erase(&cmd_list);
			biproc_erase(&proc_list);
			return 0;
		}

		job_memory_erase(SSKO);
	}

	/* Memory Erasing */

	bidir_lst_erase(&history_list);
	bidir_lst_erase(&cmd_list);
	biproc_erase(&proc_list);

	/* End */

	return 0;
}

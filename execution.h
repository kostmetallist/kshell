#ifndef __EXECUTION_H__
#define __EXECUTION_H__


void handler_all(int sig_num);

void ppline_cont(int sig_num);

int embed_cd(char **data, int arg_num);

int embed_pwd(int arg_num);

int embed_fg(char **data, int arg_num, G_data *metadata, int ground, Biproc_lst *ProcList);

int embed_bg(char **data, int arg_num, G_data *metadata, int ground, Biproc_lst *ProcList);

int embed_mcat(char **data, int arg_num);

int embed_msed(char **data, int arg_num);

int embed_mgrp(char **data, int arg_num);

int *stream_redir(char *inp_redir, char *out_redir, int out_type, int *changed_fds);

int cmd_separated_run(job **job_data, programme *prog, Bidir_lst *CmdList, Bidir_lst *HistList,
			Biproc_lst *ProcList, int ground, G_data *metadata);

int cmds_exe(job **job_begin, Bidir_lst *CmdList, Bidir_lst *HistList, Biproc_lst *ProcList,
			G_data *metadata);

#endif	/* EXECUTION_H */

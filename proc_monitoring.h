#ifndef __PROC_MONITORING_H__
#define __PROC_MONITORING_H__


void biproc_init(Biproc_lst *listname);

void biproc_append(Biproc_lst *listname, char *data, int number, int ground, int status, int pid);

void biproc_erase(Biproc_lst *listname);

proc_element *biproc_getbynumber(Biproc_lst *listname, int elem_num);

int biproc_delbynumber(Biproc_lst *listname, int elem_num);

int biproc_findbypid(Biproc_lst *listname, int pid_to_delete);

void biproc_readall(Biproc_lst *listname);

int give_term_group(int process_group);

void zombie_reaper(Biproc_lst *ProcList);

#endif /* PROC_MONITORING */

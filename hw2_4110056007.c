#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <signal.h>

#define MAXLEN 200

char bg_state[MAXLEN];
//char tt[MAXLEN];

char* replace_home(char *chkh, char *string) {
	int i, cnt = 0;
	string[cnt++] = '~';
	for (i = strlen(chkh); i < strlen(string); i++)
		string[cnt++] = string[i];
	for (i = cnt; i < strlen(string); i++)
		string[i] = '\0';
	return string;
}

char* set_export(char *string) {
	int i, cnt = 0;
	for (i = 11; i < strlen(string); i++)
		string[cnt++] = string[i];
	for (i = cnt; i < strlen(string); i++)
		string[i] = '\0';
	return string;
}

void signal_handler() {
	int state;
	pid_t pid;
	while ((pid = waitpid(-1, &state, WNOHANG)) > 0) {
		if (WIFEXITED(state))
			snprintf(bg_state, MAXLEN, "[1] %d Done\n", pid);
		else if (WIFSIGNALED(state))
			snprintf(bg_state, MAXLEN, "[1]+ %d Terminated\n", pid);
	}
}

void main() {
	pid_t pid;
	using_history();
	read_history(".history_file");
	signal(SIGCHLD, signal_handler);
	while(1) {
		char* username = getenv("USER");
		char init[4096], cmd[MAXLEN], hostname[MAXLEN], path[MAXLEN], temp[MAXLEN], chkh[MAXLEN];
		memset(chkh, '\0', MAXLEN);
		strcat(strcpy(chkh, "/home/"), username);
		gethostname(hostname, MAXLEN);
		memset(path, '\0', MAXLEN);
		if (strncmp(getcwd(path, MAXLEN), chkh, strlen(chkh)) == 0)
			strcpy(path, replace_home(chkh, path));
		sprintf(init,"%s@%s:%s$ ", username, hostname, path);
		char *input = readline(init);
		if (input == NULL)
			continue;
		strcpy(cmd, input);
		cmd[strlen(cmd)] = '\0';
		add_history(input);
		if (strcmp("exit", cmd) == 0)
			break;
		memset(temp, '\0', MAXLEN);
		strcpy(temp, cmd);
		char *arg_temp = NULL;
		int arg_cnt = 0, i, str_flag = 0, bg_flag = 0;
		arg_temp = strtok(cmd, " ");
		
		while (arg_temp != NULL) {
			//printf("arg_cnt[%d]: %s\n", arg_cnt, arg_temp);
			arg_temp = strtok(NULL, " ");
			arg_cnt++;
		}
		char* arg[arg_cnt+1];
		arg_temp = strtok(temp, " ");
		for (i = 0; i < arg_cnt; i++) {
			arg[i] = arg_temp;
			//printf("arg[%d]: .%s.\n", i, arg[i]);
			if (strcmp(arg[i], ">") == 0)
				str_flag = i;
			else if (strcmp(arg[i], "&") == 0)
				bg_flag = 1;
			arg_temp = strtok(NULL, " ");
		}
		// printf("arg_cnt = %d\n", arg_cnt);
		arg[arg_cnt] = NULL;
		if (str_flag != 0) {
			char from[MAXLEN], to[MAXLEN], buffer[MAXLEN], end[] = "\0";
			arg[str_flag] = NULL;
			strcpy(from, arg[str_flag-1]);
			strcpy(to, arg[str_flag+1]);
			arg[str_flag+1] = NULL;
			printf("sf = %d, from = .%s, to = .%s.\n", str_flag, from, to);
			FILE *f = fopen(from, "r");
			FILE *t = fopen(to, "w");
			while(fgets(buffer, MAXLEN, f) != NULL)
				fputs(buffer, t);
			//fwrite(end, 1, strlen(end), t);
			fclose(f);
			fclose(t);
			
		}
		if (strcmp(arg[0], "cd") == 0){
			if (arg[1] == NULL){
				arg[arg_cnt] = (char *)malloc(MAXLEN);
				strcpy(arg[arg_cnt], chkh);
			}
			else if (arg[1][0] == '~'){
				for (i = 0; i < strlen(arg[1]); i++)
					arg[1][i] = arg[1][i+1];
				strcat(chkh, arg[1]);
				strcpy(arg[1], chkh);
			}
			chdir(arg[1]);
		}
		else if (strcmp(arg[0], "pwd") == 0)
			printf("%s\n", getcwd(temp, MAXLEN));
		else if (strcmp(arg[0], "export") == 0) {
			printf("PATH: %s\n", getenv("PATH"));
			setenv("PATH", strcpy(arg[1], set_export(arg[1])), 1);
			printf("PATH: %s\n", getenv("PATH"));
		}
		else if (strcmp(arg[0], "echo") == 0) {
			for (i = 1; i < arg_cnt-1; i++)
				printf("%s ", arg[i]);
			printf("%s\n", arg[i]);
		}
		else if (strcmp(arg[0], "bg") == 0){
			if (kill(pid, SIGKILL) == 0)
				printf("bash: bg: job 1 already in background\n");
			printf("%s", bg_state);
		}
		else {
			//printf("this is else\n");
			pid = fork();
			if (pid == 0) {
				execvp(arg[0], arg);
				exit(0);
			}
			else {
				if (bg_flag == 0) wait(NULL);
				else printf("[1] %d\n", pid);
			}
		}
	}
	return;
}

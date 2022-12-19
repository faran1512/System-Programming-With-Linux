// Make a history.txt file in the directory where your shell is to avoid "Unable to import history" error. To create file just write "touch history.txt" on terminal
// You only have to do it once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXPIPES 20
#define MAXARGS 10
#define ARGLEN 30

int execute(char *arglist[]);
int tokenize(char *);
char *read_cmd();
int check_input_redirection_ret_index(char **);
int check_output_redirection_ret_index(char **);
bool is_external(char **);
void execute_cd(char *);
void execute_exit(void);
void execute_help(void);
void execute_jobs(void);
void handleInput(char *);
int checkPipe(char *);
int checksemicolon(char *);
int checkvariable(char *);
int checkBackground(char *);
void makePipes(void);
void printvariablelist(void);
int semicolon(char *);
int checkvariableprint(char *);
int checkHistory(char *);
void create_pipe(int **, int);
int execPipes(char *arglist[], int, int, int);
int closePipefd(char *arglist[], int, int, int);

int size_of_arglist;
int size_of_arglist1;
int fd_stdout;
int fd_stdin;
int semicolonlist_len;
int pipeslist_len;
int backgroundlist_len = 0;
char *arglist[MAXARGS];
char *arglist1[MAXARGS];
char *Inputlist[MAXARGS];
char *Outputlist[MAXARGS];
char *pipeslist[MAXPIPES];
char *semicolonlist[MAX_LEN];
char *backgroundlist[MAXARGS];
bool Background = false;
char cmd[MAX_LEN];
int cur_his = 0;

void childsignalhandler(int signum)
{
  waitpid(-1, NULL, WNOHANG);
}

int main()
{
  int rv = read_history_range("history.txt", 0, -1);
  cur_his = history_length;
  // if (rv == 0) {
  //   printf("History Imported Successfully!!!");
  //   fflush(stdout);
  // }
  if (rv != 0)
  {
    printf("Unable to Import history");
    fflush(stdout);
  }
  fd_stdout = dup(1);
  fd_stdin = dup(0);
  char *cmdline;
  while ((cmdline = read_cmd()) != NULL)
  {
    int x = semicolon(cmdline);
    if (x == 0)
    {
      handleInput(cmdline);
    }
  }
  printf("\n");
  return 0;
}

int semicolon(char *cmd)
{
  int semicolonStatus = checksemicolon(cmd);
  if (semicolonStatus)
  {
    for (int i = 0; i < semicolonlist_len; i++)
    {
      handleInput(semicolonlist[i]);
    }
  }
  return semicolonStatus;
}

void handleInput(char *cmd)
{
  int x;
  Background = false;
  int historyStatus = checkHistory(cmd);
  int backgroundStatus = checkBackground(cmd);
  int variableStatus = checkvariable(cmd);
  int variablePrintStatus = checkvariableprint(cmd);
  int pipeStatus = checkPipe(cmd);
  if (backgroundStatus)
  {
    Background = true;
    int x = tokenize(cmd);
    arglist[size_of_arglist - 1] = '\0';
    execute(arglist);
  }
  else if (variableStatus)
  {
    return;
  }
  else if (variablePrintStatus)
  {
    char *key, *rand;
    rand = strsep(&cmd, " ");
    if ((key = strsep(&cmd, " ")) != NULL)
    {
      if (getenv(key) != NULL)
      {
        printf("value : %s\n", getenv(key));
        fflush(stdout);
        return;
      }
    }
    printf("variable not found!!!\n");
    fflush(stdout);
    return;
  }
  else if (pipeStatus)
  {
    makePipes();
  }
  else if (historyStatus)
  {
    HIST_ENTRY **history = history_list();
    for (int i = 0; i < history_length; i++)
    {
      printf("%s\n", history[i]->line);
      fflush(stdout);
    }
  }
  else
  {
    x = tokenize(cmd);
    if (x == 0)
    {
      return;
    }
    bool ans = is_external(arglist);
    if (ans)
    {
      if (x == 1)
      {
        execute(arglist);
      }
      else if (x == 2)
      {
        execute(Inputlist);
      }
    }
  }
}

void makePipes()
{
  int no_of_pipes = pipeslist_len - 1;
  int pipefd[no_of_pipes][2];

  pipe(pipefd[0]);
  tokenize(pipeslist[0]);

  closePipefd(arglist, 0, -1, pipefd[0][1]);
  for (int i = 1; i < no_of_pipes; i++)
  {
    pipe(pipefd[i]);
    tokenize(pipeslist[i]);

    closePipefd(arglist, 0, pipefd[i - 1][0], pipefd[i][1]);
  }

  tokenize(pipeslist[no_of_pipes]);

  closePipefd(arglist, 1, pipefd[no_of_pipes - 1][0], -1);

  // for(int i = 0 ; i < no_of_pipes + 1 ; i++)
  // {
  //     wait(NULL);
  // }
}

int closePipefd(char *arglist[], int wait_flag, int in, int out)
{
  int rv = execPipes(arglist, wait_flag, in, out);
  if (in != -1)
  {
    close(in);
  }
  if (out != -1)
  {
    close(out);
  }
  return rv;
}

int execPipes(char *arglist[], int wait_flag, int in, int out)
{
  int status;
  int cpid = fork();
  switch (cpid)
  {
  case -1:
    perror("fork failed");
    exit(1);
  case 0:
    if (in != -1)
    {
      dup2(in, 0);
    }
    if (out != -1)
    {
      dup2(out, 1);
    }
    execvp(arglist[0], arglist);
    perror("Command not found...");
    exit(1);
  default:
    wait(NULL);
    return 0;
  }
}

int execute(char *arglist[])
{
  int status;
  int cpid = fork();
  switch (cpid)
  {
  case -1:
    perror("fork failed");
    exit(1);
  case 0:
    execvp(arglist[0], arglist);
    perror("Command not found...");
    exit(1);
  default:
    if (Background)
    {
      signal(SIGCHLD, childsignalhandler);
      // waitpid(cpid, NULL, WNOHANG);
      // kill(cpid, SIGTSTP);
      // kill(cpid, SIGCONT);
    }
    else
    {
      waitpid(cpid, &status, 0);
      dup2(fd_stdout, 1);
      dup2(fd_stdin, 0);
      printf("child exited with status %d \n", status >> 8);
    }
    return 0;
  }
}

int checkPipe(char *str)
{
  pipeslist_len = 0;
  int i;
  for (i = 0; i < MAX_LEN; i++)
  {
    pipeslist[i] = strsep(&str, "|");
    if (pipeslist[i] == NULL)
    {
      break;
    }
    pipeslist_len++;
  }
  if (pipeslist[1] == NULL)
    return 0;
  else
  {
    return 1;
  }
}

int checkHistory(char *str)
{
  char *match;
  match = strstr(str, "history");
  if (match)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int checksemicolon(char *str)
{
  int i;
  for (i = 0; i < MAX_LEN; i++)
  {
    semicolonlist[i] = strsep(&str, ";");
    if (semicolonlist[i] == NULL)
    {
      semicolonlist_len = i;
      break;
    }
  }
  if (semicolonlist[0] == NULL)
    return 0;
  else
  {
    semicolonlist_len = i;
    return 1;
  }
}

int checkvariable(char *str)
{
  char *key, *value, *temp;
  key = strsep(&str, "=");
  value = strsep(&str, "=");
  if (key == NULL || value == NULL)
  {
    return 0;
  }
  else
  {
    if ((temp = strsep(&key, " ")) != NULL)
    {
      setenv(temp, value, 1);
    }
    else
    {
      setenv(key, value, 1);
    }
    return 1;
  }
}

int checkvariableprint(char *str)
{
  char *match;
  match = strstr(str, "print");
  if (match)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int checkBackground(char *str)
{
  char *ch;
  ch = strchr(str, '&');
  if (ch != NULL)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int tokenize(char *cmdline)
{
  if (cmdline[0] == '\0')
    return 0;
  int f;
  for (f = 0; f < MAXARGS; f++)
  {
    arglist[f] = strsep(&cmdline, " ");
    if (arglist[f] == NULL)
    {
      break;
    }
    if (strlen(arglist[f]) == 0)
    {
      f--;
    }
  }
  size_of_arglist = f;
  int in = check_input_redirection_ret_index(arglist);
  int out = check_output_redirection_ret_index(arglist);
  if (in == -1 && out == -1)
  {
    return 1;
  }
  else if (out >= 0 && in >= 0)
  {
    if (in < out)
    {
      int q;
      for (q = 0; q < in; q++)
      {
        Inputlist[q] = arglist[q];
      }
      size_of_arglist = q;
      return 2;
    }
    else if (out < in)
    {
      int q;
      for (q = 0; q < out; q++)
      {
        Inputlist[q] = arglist[q];
      }
      size_of_arglist = q;
      return 2;
    }
  }
  else if (out >= 0 && in == -1)
  { // echo hello > f1.txt
    int q;
    for (q = 0; q < out; q++)
    {
      Inputlist[q] = arglist[q];
    }
    size_of_arglist = q;
    return 2;
  }
  else if (in >= 0 && out == -1)
  { // cat < /etc/passwd
    int q;
    for (q = 0; q < in; q++)
    {
      Inputlist[q] = arglist[q];
    }
    size_of_arglist = q;
    return 2;
  }
}

char *read_cmd()
{
  char *buf;
  char cwd[256];
  char x[] = "~ ";
  getcwd(cwd, sizeof(cwd));
  buf = readline(strcat(cwd, x));
  if (strlen(buf) != 0)
  {
    add_history(buf);
    strcpy(cmd, buf);
    return cmd;
  }
}

bool is_external(char **command)
{
  if (strcmp(command[0], "cd") == 0)
  {
    execute_cd(command[1]);
  }
  else if (strcmp(command[0], "exit") == 0)
  {
    execute_exit();
  }
  else if (strcmp(command[0], "help") == 0)
  {
    execute_help();
  }
  else if (strcmp(command[0], "jobs") == 0)
  {
    execute_jobs();
  }
  else
  {
    return true;
  }
  return false;
}

void execute_cd(char *cmd)
{
  int rv;
  if ((rv = chdir(cmd) != 0))
  {
    perror("Directory not found...");
  }
}

void execute_exit()
{
  int offset = history_length - cur_his;
  int rv = append_history(offset, "history.txt");
  // if (rv == 0) {
  //   printf("History Saved Successfully!!!");
  //   fflush(stdout);
  // }
  if (rv != 0)
  {
    printf("Unable to save History!!!");
    fflush(stdout);
  }
  exit(0);
}

void execute_help()
{
  printf("This is help for my shell\n");
  return;
}

void execute_jobs()
{
  system("jobs");
  return;
}

int check_input_redirection_ret_index(char **cmd)
{
  int fd;
  for (int i = 0; i < size_of_arglist; i++)
  {
    if ((strcmp(cmd[i], "<") == 0))
    {
      fd = open(cmd[i + 1], O_RDONLY);
      dup2(fd, 0);
      return i;
    }
  }
  return -1;
}

int check_output_redirection_ret_index(char **cmd)
{
  int fd;
  for (int i = 0; i < size_of_arglist; i++)
  {
    if ((strcmp(cmd[i], ">") == 0))
    {
      fd = open(cmd[i + 1], O_RDWR | O_CREAT);
      dup2(fd, 1);
      return i;
    }
  }
  return -1;
}

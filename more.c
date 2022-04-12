#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

static int LINE_L;
static int LINES;
static char fileName[300];
static int count_of_lines;
static int current_count;
static int page_count;

void show(FILE *, int);
int get_input(double, FILE *);
void count_lines(FILE *);

int main(int argc, char *argv[])
{
  LINES = atoi(getenv("LINES"));
  LINE_L = atoi(getenv("COLUMNS"));
  bool flag = false;
  char container[LINE_L];
  if (argc == 1 && !system("test -t 0"))
  {
    printf("more: bad usage\nTry 'more --help' for more information.\n");
  }
  else if (argc == 1 && system("test -t 0"))
  {
    char buffer[LINE_L], file[LINE_L];
    int pid = getpid();
    int fd = fileno(stdin);
    snprintf(buffer, LINE_L, "/proc/self/fd/%d", fd);
    int count = readlink(buffer, file, 100);
    file[count] = '\0';
    dup2(1, 0);
    strcpy(fileName, file);
    FILE *p = fopen(file, "r");
    count_lines(p);
    show(p, 0);
  }
  else if (argc > 2)
  {
    flag = true;
  }
  if (argc > 1)
  {
    FILE *fp;
    for (int i = 1; i < argc; i++)
    {
      fp = fopen(argv[i], "r");
      if (fp == NULL)
      {
        perror("Can't open file");
        exit(1);
      }
      if (flag == true)
      {
        printf("::::::::::::::\n");
        printf("%s\n", argv[i]);
        printf("::::::::::::::\n");
      }
      strcpy(fileName, argv[i]);
      count_lines(fp);
      show(fp, 0);
      fclose(fp);
    }
    flag = false;
  }
  return 0;
}

void search(FILE *fp)
{
  bool f = true;
  int len, ans;
  char buffer[50], temp[LINE_L];
  fgets(buffer, 50, stdin);
  len = ftell(fp);
  int temp_count = current_count;
  while (fgets(temp, LINE_L, fp))
  {
    current_count++;
    if (strstr(temp, buffer) != NULL)
    {
      fseek(fp, ans, SEEK_SET);
      f = false;
      printf("...skipping\n");
      show(fp, 2);
    }
    ans = ftell(fp);
  }
  if (f)
  {
    current_count = temp_count;
    fseek(fp, len, SEEK_SET);
    show(fp, 0);
  }
}

void count_lines(FILE *fp)
{
  count_of_lines = 0;
  current_count = 0;
  char container[LINE_L];
  while (fgets(container, LINE_L, fp))
  {
    count_of_lines++;
  }
  fseek(fp, 0, SEEK_SET);
}

void show(FILE *fp, int page_count)
{
  int val;
  double percentage;
  char buffer[LINE_L];
  while (fgets(buffer, LINE_L, fp))
  {
    fputs(buffer, stdout);
    page_count++;
    current_count++;
    if (page_count == LINES - 1)
    {
      percentage = ((double)current_count / (double)count_of_lines) * 100.0;
      val = get_input(percentage, fp);
      if (val == 0)
      {
        printf("\033[2K \033[1G");
        exit(0);
      }
      else if (val == 1)
      {
        printf("\n");
        printf("\033[1A \033[2K \033[1G");
        page_count -= LINES;
      }
      else if (val == 2)
      {
        printf("\n");
        printf("\033[1A \033[2K \033[1G");
        page_count -= 1;
      }
      else if (val == 5)
      {
        printf("\033[1A \033[2K \033[1G");
        break;
      }
    }
  }
}

int get_input(double per, FILE *fp)
{
  FILE *terminal = fopen("/dev/tty", "r");
  char c;
  printf("\033[7m--More--(%.lf%%)\033[m", per);
  struct termios info;
  tcgetattr(0, &info);
  struct termios save = info;
  info.c_lflag &= ~ICANON;
  info.c_lflag &= ~ECHO;
  tcsetattr(0, TCSANOW, &info);
  c = getc(terminal);
  tcsetattr(0, TCSANOW, &save);
  if (c == 'q')
  {
    return 0;
  }
  else if (c == ' ')
  {
    return 1;
  }
  else if (c == '\n')
  {
    return 2;
  }
  else if (c == '/')
  {
    printf("\033[2K \033[1G");
    printf("/");
    search(fp);
    return 3;
  }
  else if (c == 'v')
  {
    char command[LINE_L];
    strcpy(command, "vim ");
    strcat(command, fileName);
    printf("%s", command);
    system(command);
    printf("\033[2K \033[1G");
    show(fp, LINES - 2);
    return 4;
  }
  else
  {
    return 5;
  }
  return 0;
}

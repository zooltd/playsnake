#include <arpa/inet.h>
#include <curses.h>
#include <errno.h>
#include <error.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#define OOPS(msg)                                                              \
  {                                                                            \
    perror(msg);                                                               \
    return 1;                                                                  \
  }
#define HEIGHT 24
#define WIDTH 80
#define FOOD -100
#define PORT 9999

WINDOW *win;
int game_result = 1;
// int key = -1;
char *log_path = "log.txt";

int add_thread(void *(*)(void *), void *);

void *update_screen(void *);

void add_log(const char *);

void init_log();

int main(int argc, char *argv[]) {
  init_log();

  int sockfd;
  struct sockaddr_in serv_addr;
  if (argc != 2)
    error(1, 0, "usage: tcpclient <IPaddress>");

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
  int connect_rt =
      connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connect_rt < 0) {
    error(1, errno, "connect failed ");
  }

  initscr();
  clear();
  noecho();
  cbreak();
  start_color();
  use_default_colors();
  curs_set(0);

  // Set window to new ncurses window
  win = newwin(HEIGHT, WIDTH, 0, 0);

  // Snake colours
  init_pair(1, COLOR_WHITE, COLOR_RED);
  init_pair(5, COLOR_BLACK, COLOR_YELLOW);
  init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
  init_pair(7, COLOR_BLACK, COLOR_CYAN);
  init_pair(8, COLOR_BLACK, COLOR_WHITE);

  add_thread(update_screen, &sockfd);
  add_log("thread added.\n");

  int key_buffer;
  while (game_result) {
    bzero(&key_buffer, sizeof(key_buffer));
    key_buffer = wgetch(win);
    if (key_buffer == 'w' || key_buffer == 's' || key_buffer == 'a' ||
        key_buffer == 'd') {
      int n = write(sockfd, &key_buffer, sizeof(key_buffer));
      if (n < 0)
        error(1, errno, "ERROR writing to socket.");
      add_log("have written to the server.\n");
    }
  }

  add_log("gonna to generate announcement\n");
  // Show the user who won
  WINDOW *announcement = newwin(7, 35, (HEIGHT - 7) / 2, (WIDTH - 35) / 2);
  box(announcement, 0, 0);
  mvwaddstr(announcement, 2, (35 - 21) / 2, "Game Over");
  mvwaddstr(announcement, 4, (35 - 21) / 2, "Press any key to quit.");
  wbkgd(announcement, COLOR_PAIR(1));
  mvwin(announcement, (HEIGHT - 7) / 2, (WIDTH - 35) / 2);
  wnoutrefresh(announcement);
  wrefresh(announcement);
  sleep(2);
  wgetch(announcement);
  delwin(announcement);
  wclear(win);
  echo();
  curs_set(1);
  endwin();
  // Close connection
  close(sockfd);
  return 0;
}

int add_thread(void *(*fn)(void *), void *arg) {
  pthread_attr_t attr;
  pthread_t tid;
  if (pthread_attr_init(&attr) != 0)
    OOPS("pthread_attr_init");
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&tid, &attr, fn, arg) != 0)
    OOPS("pthread_create");
  pthread_attr_destroy(&attr);
  return 0;
};

void *update_screen(void *arg) {
  int sockfd = *(int *)arg;
  int bytes_read;
  int map[HEIGHT][WIDTH];
  int map_size = HEIGHT * WIDTH * sizeof(map[0][0]);
  char map_buffer[map_size];
  int i, j, n;

  while (game_result) {
    // receive updated map from server
    bytes_read = 0;
    bzero(map_buffer, map_size);
    while (bytes_read < map_size) {
      n = read(sockfd, map_buffer + bytes_read, map_size - bytes_read);
      if (n <= 0)
        goto end;
      bytes_read += n;
    }
    add_log("have read from the server.\n");
    memcpy(map, map_buffer, map_size);

    clear();
    box(win, 0, 0);
    refresh();
    wrefresh(win);
    for (i = 1; i < HEIGHT - 1; i++) {
      for (j = 1; j < WIDTH - 1; j++) {
        int current = map[i][j];
        int colour = abs(current) % 7;
        attron(COLOR_PAIR(colour));
        if ((current > 0) && (current != FOOD)) {
          mvprintw(i, j, " ");
          attroff(COLOR_PAIR(colour));
        } else if (current == FOOD) {
          attroff(COLOR_PAIR(colour));
          mvprintw(i, j, "o");
        }
      }
    }
    refresh();
  }
end:
  game_result = 0;
  add_log("thread <update_screen> over\n");
  return 0;
}

void add_log(const char *str) {

  FILE *fp = fopen(log_path, "a+");
  if (fp == 0) {
    printf("can't open log file\n");
    return;
  }
  fseek(fp, 0, SEEK_END);
  fwrite(str, strlen(str), 1, fp);
  fclose(fp);
}

void init_log() {
  remove(log_path);
  time_t tt = time(0);
  char s[21];
  strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", localtime(&tt));
  s[19] = '\n';
  s[20] = '\0';
  add_log(s);
}
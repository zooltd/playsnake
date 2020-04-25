#include "game.h"
#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <error.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define REFRESH 0.15

#define OOPS(msg) {perror(msg);return 1;}
#define PORT 9999

int players = 0;//人数
tpool_t *pool;

pthread_mutex_t map_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int no;
    direction dir;
} modify_arg;

void make_non_blocking(int fd);

int tcp_non_blocking_server_listen(int port);

void *modify_snake_direction(void *);

int run_thread();

void *run();

void quit_handler();

int add_snake(int);

int can_change_direction(direction direc, snake *player_snake);

int main(void) {
    signal(SIGINT, quit_handler);
    int listen_fd, socket_fd;
    int efd;
    int event_num;
    struct epoll_event event;
    struct epoll_event *events;
    int i;

    listen_fd = tcp_non_blocking_server_listen(PORT);

    //初始化地图，生成食物和边界
    init_map();

    //初始化线程池
    if (create_tpool(&pool, 4) != 0) {
        fprintf(stdout, "create thread pool failed!\n");
        return -1;
    }

    //运行常驻线程
    if (run_thread() != 0) {
        fprintf(stdout, "create thread failed!\n");
        return -1;
    };

    for (i = 0; i < MAXSIZE; ++i) {
        info[i].player_no = -1;
    }

    efd = epoll_create1(0);
    if (efd == -1) error(1, errno, "epoll create failed");

    // 将监听套接字对应的 I/O 事件进行了注册
    event.data.fd = listen_fd;
    event.events = EPOLLIN | EPOLLET; //edge-triggered
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &event) == -1) error(1, errno, "epoll_ctl add listen fd failed");

    /* Buffer where events are returned */
    events = calloc(MAXSIZE, sizeof(event));

    while (1) {
        event_num = epoll_wait(efd, events, MAXSIZE, -1);
        printf("epoll_wait wakeup\n");
        for (i = 0; i < event_num; i++) {
            //判断错误情况
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            } else if (listen_fd == events[i].data.fd) { //监听到连接
                struct sockaddr_storage ss;
                socklen_t slen = sizeof(ss);
                int fd = accept(listen_fd, (struct sockaddr *) &ss, &slen); //调用 accept 获取已建立连接
                if (fd < 0) {
                    error(1, errno, "accept failed");
                } else {
                    make_non_blocking(fd);
                    event.data.fd = fd;
                    event.events = EPOLLIN | EPOLLET; //FIXME
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event) == -1) { //调用 epoll_ctl 把已连接套接字对应的可读事件注册到 epoll 实例中
                        error(1, errno, "epoll_ctl add connection fd failed");
                    }
                    add_snake(fd);//TODO
                    players++;
                    fprintf(stdout, "%d player comes.\n", fd);
                }
                continue;
            } else {    //监听到client可操作（可读）
                socket_fd = events[i].data.fd;
                fprintf(stdout, "player %d inputs stn.\n", socket_fd);
                direction buf;
                int n;
                if ((n = read(socket_fd, &buf, sizeof(buf))) < 0) {
                    if (errno != EAGAIN) {
                        error(1, errno, "read error");
                        close(socket_fd);
                    }
                    break;
                } else if (n == 0) {
                    close(socket_fd);
                    break;
                } else {
                    fprintf(stdout, "player %d input %d\n", socket_fd, buf);
                    modify_arg args = {socket_fd, buf};
                    add_task_2_tpool(pool, &modify_snake_direction, &args);
                }
            }
        }
    }
}

void *modify_snake_direction(void *arg) {
    modify_arg args = *(modify_arg *) arg;
    direction direction = args.dir;
    int player_no = args.no;
    for (int i = 0; i < MAXSIZE; ++i) {
        if ((info[i].player_no == player_no) && can_change_direction(direction, info[i].player_snake)) {
            info[i].player_snake->head->dir = direction;
            break;
        }
    }
    return 0;
}

int can_change_direction(direction direc, snake *player_snake) {
    return (((direc == UP) && (player_snake->head->dir != DOWN)) ||
            ((direc == DOWN) && (player_snake->head->dir != UP)) ||
            ((direc == LEFT) && (player_snake->head->dir != RIGHT)) ||
            ((direc == RIGHT) && (player_snake->head->dir != LEFT)));
}

int run_thread() {
    pthread_attr_t attr;
    pthread_t tid;
    if (pthread_attr_init(&attr) != 0) OOPS("pthread_attr_init")
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, &run, NULL) != 0) OOPS("pthread_create")
    pthread_attr_destroy(&attr);
    return 0;
}

void *run() {
    struct timespec ts;
    ts.tv_sec = (__time_t) REFRESH;
    ts.tv_nsec = ((int) (REFRESH * 1000) % 1000) * 1000000;
    while (1) {
        if (players <= 0) { continue; }
        char map_buffer[MAP_SIZE];
        memcpy(map_buffer, map, MAP_SIZE);
        pthread_mutex_lock(&map_lock);
        // broadcast
        for (int i = 0; i < MAXSIZE; ++i) {
            if (info[i].player_no >= 0) {
                int bytes_sent = 0;
                while (bytes_sent < MAP_SIZE) {
                    bytes_sent += write(info[i].player_no, map_buffer, MAP_SIZE);
                    if (bytes_sent < 0) {
                        perror("ERROR writing to socket");
                        break;
                    }
                }
                fprintf(stdout, "have written to player %d.\n", info[i].player_no);
            }
        }
        // snakes act
        for (int i = 0; i < MAXSIZE; ++i) {
            if (info[i].player_no >= 0) {
                if (action(info[i].player_snake)) {
                    players--;
                    kill_snake(info[i].player_snake);
                    fprintf(stdout, "gonna kill player %d.\n", info[i].player_no);
                    close(info[i].player_no);
                    info[i].player_no = -1;
                } else {
                    fprintf(stdout, "player %d moves.\n", info[i].player_no);
                }
            }
        }
        pthread_mutex_unlock(&map_lock);
        nanosleep(&ts, NULL);
    }
}

int add_snake(int fd) {
    pthread_mutex_lock(&map_lock);
    for (int i = 0; i < MAXSIZE; ++i) {
        if (info[i].player_no < 0) {
            info[i].player_no = fd;
            info[i].player_snake = make_snake(fd);
            pthread_mutex_unlock(&map_lock); //fixed
            return 1;
        }
    }
    pthread_mutex_unlock(&map_lock);
    fprintf(stdout, "release mutex on add_snake.\n");
    return 0;
}

void quit_handler() {
    printf("\nThe server is killed.\n");
    exit(0);
}


void make_non_blocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

int tcp_non_blocking_server_listen(int port) {
    int listen_fd;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    make_non_blocking(listen_fd);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listen_fd, 128);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGPIPE, SIG_IGN);

    return listen_fd;
}
#ifndef SERVER_GAME_H
#define SERVER_GAME_H
#define HEIGHT 24
#define WIDTH 80
#define FOOD_NUM 5
#define WINNER_LENGTH 20
#define BORDER -99
#define FOOD -100
#define MAXSIZE 5
#define MAP_SIZE WIDTH*HEIGHT*sizeof(map[0][0])

typedef enum {
    UP = 'w',
    DOWN = 's',
    LEFT = 'a',
    RIGHT = 'd'
} direction;

typedef struct node node_t;

struct node {
    int y, x;
    direction dir;
    node_t *next;
};

typedef struct {
    node_t *head;
    node_t *tail;
    int length;
    int no;
} snake;

typedef struct {
    int player_no;
    snake *player_snake;
} game_info;

game_info info[MAXSIZE];

int map[HEIGHT][WIDTH];

void init_map();

snake *make_snake(int);

void add_at_head(snake *);

void move(snake *);

void kill_snake(snake *);

int action(snake *);

#endif //SERVER_GAME_H

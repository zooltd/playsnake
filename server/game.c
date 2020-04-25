#include "game.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

direction choose[] = {UP, DOWN, LEFT, RIGHT};

int not_all_zero(int start_y, int start_x, int height, int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (map[start_y + i][start_x + j] != 0) return 1;
        }
    }
    return 0;
}

int fill_in(int start_y, int start_x, int height, int width, int number) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            map[start_y + i][start_x + j] = number;
        }
    }
    return 0;
}

snake *make_snake(int player_no) {
    int head_y, head_x;
    srand(time(NULL));
    int choose_index = rand() % 4;
    direction init_dir = choose[choose_index];
    node_t *head_node = malloc(sizeof *head_node);
    node_t *seg_node = malloc(sizeof *seg_node);
    node_t *tail_node = malloc(sizeof *tail_node);
    switch (init_dir) {
        case UP:
            do {
                head_y = rand() % (HEIGHT - 10) + 5;
                head_x = rand() % (WIDTH - 10) + 5;
            } while (not_all_zero(head_y, head_x, 3, 2));
            head_node->y = head_y;
            head_node->x = head_x;
            head_node->dir = UP;
            head_node->next = seg_node;
            seg_node->y = head_y + 1;
            seg_node->x = head_x;
            seg_node->dir = UP;
            seg_node->next = tail_node;
            tail_node->y = head_y + 2;
            tail_node->x = head_x;
            tail_node->dir = UP;
            tail_node->next = NULL;
            fill_in(head_y, head_x, 3, 2, player_no);
            break;
        case DOWN:
            do {
                head_y = rand() % (HEIGHT - 10) + 5;
                head_x = rand() % (WIDTH - 10) + 5;
            } while (not_all_zero(head_y - 2, head_x, 3, 2));
            head_node->y = head_y;
            head_node->x = head_x;
            head_node->dir = DOWN;
            head_node->next = seg_node;
            seg_node->y = head_y - 1;
            seg_node->x = head_x;
            seg_node->dir = DOWN;
            seg_node->next = tail_node;
            tail_node->y = head_y - 2;
            tail_node->x = head_x;
            tail_node->dir = DOWN;
            tail_node->next = NULL;
            fill_in(head_y - 2, head_x, 3, 2, player_no);
            break;
        case LEFT:
            do {
                head_y = rand() % (HEIGHT - 10) + 5;
                head_x = rand() % (WIDTH - 10) + 5;
            } while (not_all_zero(head_y, head_x, 1, 6));
            head_node->y = head_y;
            head_node->x = head_x;
            head_node->dir = LEFT;
            head_node->next = seg_node;
            seg_node->y = head_y;
            seg_node->x = head_x + 2;
            seg_node->dir = LEFT;
            seg_node->next = tail_node;
            tail_node->y = head_y;
            tail_node->x = head_x + 4;
            tail_node->dir = LEFT;
            tail_node->next = NULL;
            fill_in(head_y, head_x, 1, 6, player_no);
            break;
        case RIGHT:
            do {
                head_y = rand() % (HEIGHT - 10) + 5;
                head_x = rand() % (WIDTH - 10) + 5;
            } while (not_all_zero(head_y, head_x - 4, 1, 6));
            head_node->y = head_y;
            head_node->x = head_x;
            head_node->dir = RIGHT;
            head_node->next = seg_node;
            seg_node->y = head_y;
            seg_node->x = head_x - 2;
            seg_node->dir = RIGHT;
            seg_node->next = tail_node;
            tail_node->y = head_y;
            tail_node->x = head_x - 4;
            tail_node->dir = LEFT;
            tail_node->next = NULL;
            fill_in(head_y, head_x - 4, 1, 6, player_no);
            break;
    }
    snake *s = calloc(1, sizeof *s);
    s->head = head_node;
    s->tail = tail_node;
    s->no = player_no;
    s->length = 3;
    return s;
}

//function to randomly add a fruit to the game map
void add_food() {
    srand(time(NULL));
    int x, y;
    do {
        y = rand() % (HEIGHT - 14) + 7;
        x = rand() % (WIDTH - 14) + 7;
    } while (map[y][x] != 0);
    map[y][x] = FOOD;
}

void init_map() {
    // fill in the map array with 0
    memset(map, 0, sizeof(map));
    // define the border
    for (int i = 0; i < HEIGHT; i++)
        map[i][0] = map[i][WIDTH - 1] = BORDER;
    for (int i = 0; i < WIDTH; i++)
        map[0][i] = map[HEIGHT - 1][i] = BORDER;
    // generate the initial food
    for (int i = 0; i < FOOD_NUM; ++i) {
        add_food();
    }
}

//eat food
void add_at_head(snake *s) {
    direction new_dir = s->head->dir;
    node_t *new_node = malloc(sizeof *new_node);
    new_node->dir = new_dir;
    new_node->next = s->head;
    switch (new_dir) {
        case UP:
            new_node->y = s->head->y - 1;
            new_node->x = s->head->x;
            break;
        case DOWN:
            new_node->y = s->head->y + 1;
            new_node->x = s->head->x;
            break;
        case LEFT:
            new_node->y = s->head->y;
            new_node->x = s->head->x - 2;

            break;
        case RIGHT:
            new_node->y = s->head->y;
            new_node->x = s->head->x + 2;
            break;
        default:
            break;
    }
    s->head = new_node;
    s->length = s->length + 1;
    map[new_node->y][new_node->x] = map[new_node->y][new_node->x + 1] = s->no;
}

void move(snake *s) {
    add_at_head(s);
    node_t *iter = s->head;
    while (iter->next->next != NULL) {

        iter = iter->next;
    }
    map[s->tail->y][s->tail->x] = map[s->tail->y][s->tail->x + 1] = 0;
    iter->next = NULL;
    free(s->tail);
    s->tail = iter;
    s->length = s->length - 1;
}

void kill_snake(snake *s) {
    node_t *iter = s->head;
    node_t *temp;
    while (iter != NULL) {
        map[iter->y][iter->x] = map[iter->y][iter->x + 1] = 0;
        temp = iter->next;
        free(iter);
        iter = temp;
    }
    free(s);
}

int can_move(int para1, int para2) {
    return (para1 == 0) && (para2 == 0);
}

int can_eat(int para1, int para2) {
    return ((para1 == FOOD) && (para2 == FOOD)) || ((para1 == FOOD) && (para2 == 0)) ||
           ((para1 == 0) && (para2 == FOOD));
}

/**
 *
 * @param player_snake
 * @param key
 * @return 0:go on 1: game over
 */
int action(snake *player_snake) {
    direction key = player_snake->head->dir;
    switch (key) {
        case UP: {
            int left_up = map[player_snake->head->y - 1][player_snake->head->x];
            int right_up = map[player_snake->head->y - 1][player_snake->head->x + 1];
            if (can_move(left_up, right_up)) {
                move(player_snake);
                fprintf(stdout, "Server: snake No.%d move up.\n", player_snake->no);
            } else if (can_eat(left_up, right_up)) {
                add_at_head(player_snake);
                add_food();
                fprintf(stdout, "Server: snake No.%d eats up.\n", player_snake->no);
            } else {
                fprintf(stdout, "Server: snake No.%d dies.\n", player_snake->no);
                return 1;
            }
            break;
        }
        case DOWN: {
            int left_down = map[player_snake->head->y + 1][player_snake->head->x];
            int right_down = map[player_snake->head->y + 1][player_snake->head->x + 1];
            if ((left_down == 0) && (right_down == 0)) {
                move(player_snake);
                fprintf(stdout, "Server: snake No.%d move down.\n", player_snake->no);
            } else if (can_eat(left_down, right_down)) {
                add_at_head(player_snake);
                add_food();
                fprintf(stdout, "Server: snake No.%d eats down.\n", player_snake->no);
            } else {
                fprintf(stdout, "Server: snake No.%d dies.\n", player_snake->no);
                return 1;
            }
            break;
        }
        case LEFT: {
            int left_left = map[player_snake->head->y][player_snake->head->x - 2];
            int left = map[player_snake->head->y][player_snake->head->x - 1];
            if (can_move(left_left, left)) {
                move(player_snake);
                fprintf(stdout, "Server: snake No.%d move left.\n", player_snake->no);
            } else if (can_eat(left_left, left)) {
                add_at_head(player_snake);
                add_food();
                fprintf(stdout, "Server: snake No.%d eats left.\n", player_snake->no);
            } else {
                fprintf(stdout, "Server: snake No.%d dies.\n", player_snake->no);
                return 1;
            }
            break;
        }
        case RIGHT: {
            int right_right = map[player_snake->head->y][player_snake->head->x + 3];
            int right = map[player_snake->head->y][player_snake->head->x + 2];
            if (can_move(right_right, right)) {
                move(player_snake);
                fprintf(stdout, "Server: snake No.%d move right.\n", player_snake->no);
            } else if (can_eat(right_right, right)) {
                add_at_head(player_snake);
                add_food();
                fprintf(stdout, "Server: snake No.%d eats right.\n", player_snake->no);
            } else {
                fprintf(stdout, "Server: snake No.%d dies.\n", player_snake->no);
                return 1;
            }
            break;
        }
    }
    if (player_snake->length >= WINNER_LENGTH) {
        fprintf(stdout, "Server: snake No.%d wins.\n", player_snake->no);
        return 1;
    }
    return 0;
}






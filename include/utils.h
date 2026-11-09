#ifndef UTILS_H
#define UTILS_H

#define MAP_WIDTH 16
#define MAP_HEIGHT 10

typedef struct s_player
{
    double pos_x;
    double pos_y;
    double dir_x;
    double dir_y;
} t_player;


typedef struct s_game_state
{
    int map[MAP_HEIGHT][MAP_WIDTH];
    t_player *player;
} t_game_state;

typedef enum e_wall{
    VOID,
    WALL
} t_wall;

typedef enum e_game_input{
    UP = 'Z',
    LEFT = 'Q',
    DOWN = 'S',
    RIGHT = 'D',
    EXIT = 'X',
} t_game_input;

#endif
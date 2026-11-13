#ifndef MAP_H
#define MAP_H

#include"utils.h"
#include<stdbool.h>

void build_map(t_game_state *game_state);

void print_map(t_game_state *game_state);

bool is_wall(t_game_state *game_state, double x, double y);

#endif
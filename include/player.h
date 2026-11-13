#ifndef PLAYER_H
#define PLAYER_H

#include"utils.h"

void build_player(t_game_state *game_state);

void rotate_player(t_game_state *game_input, double angle);

void move_player(t_game_state *game_state, double deplacement);
#endif
#include<math.h>
#include"player.h"
#include"utils.h"
#include"map.h"

void build_player(t_game_state *game_state){
    game_state->player->pos_y = MAP_HEIGHT/2 + 0.5;
    game_state->player->pos_x = MAP_WIDTH/2 + 0.5;
    game_state->player->dir_x = 1;
    game_state->player->dir_y = 0;
}

void rotate_player(t_game_state *game_state, double angle){
    double new_y = game_state->player->dir_x * sin(angle) + game_state->player->dir_y * cos(angle);
    game_state->player->dir_x = game_state->player->dir_x * cos(angle) - game_state->player->dir_y * sin(angle);
    game_state->player->dir_y = new_y;
}

void move_player(t_game_state *game_state, double deplacement){

    double wanted_player_pos_x = game_state->player->pos_x + game_state->player->dir_x * deplacement;
    double wanted_player_pos_y = game_state->player->pos_y + game_state->player->dir_y * deplacement;

    if (!is_wall(game_state, game_state->player->pos_x, wanted_player_pos_y)){
        game_state->player->pos_y = wanted_player_pos_y;
    }
    if (!is_wall(game_state, wanted_player_pos_x, game_state->player->pos_y)){
        game_state->player->pos_x = wanted_player_pos_x;
    }
    
}
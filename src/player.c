#include"player.h"
#include"utils.h"

void build_player(t_game_state *game_state){
    game_state->player->pos_y = MAP_HEIGHT/2;
    game_state->player->pos_x = MAP_WIDTH/2;
}
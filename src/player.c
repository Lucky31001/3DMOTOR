#include"player.h"
#include"utils.h"

void build_player(t_game_state *game_state){
    game_state->player->pos_y = MAP_HEIGHT/2;
    game_state->player->pos_x = MAP_WIDTH/2;
}

void move_player(t_game_state *game_state, char move){

    switch (move)
        {
        case UP:
            game_state->player->pos_y--;
            break;
        case LEFT:
            game_state->player->pos_x--;
            break;
        case DOWN:
            game_state->player->pos_y++;
            break;
        case RIGHT:
            game_state->player->pos_x++;
            break;
        default:
            break;
        }
}
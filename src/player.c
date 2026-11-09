#include<math.h>
#include"player.h"
#include"utils.h"

void build_player(t_game_state *game_state){
    game_state->player->pos_y = MAP_HEIGHT/2 + 0.5;
    game_state->player->pos_x = MAP_WIDTH/2 + 0.5;
}

void move_player(t_game_state *game_state, char move){
    double player_pos_x = game_state->player->pos_x;
    double player_pos_y = game_state->player->pos_y;

    switch (move)
        {
        case UP:
            player_pos_y--;
            break;
        case LEFT:
            player_pos_x--;
            break;
        case DOWN:
            player_pos_y++;
            break;
        case RIGHT:
            player_pos_x++;
            break;
        default:
            break;
        }
    if (game_state->map[(int)floor(player_pos_y)][(int)floor(player_pos_x)] == VOID){

        game_state->player->dir_x = player_pos_x - game_state->player->pos_x;
        game_state->player->dir_y = player_pos_y - game_state->player->pos_y;
        
        game_state->player->pos_x = player_pos_x;
        game_state->player->pos_y = player_pos_y;
    }
}
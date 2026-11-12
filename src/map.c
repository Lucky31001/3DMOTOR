#include<stdio.h>
#include"map.h"
#include"utils.h"
#include <math.h>

void build_map(t_game_state *game_state){
    for(int y = 0; y < MAP_HEIGHT; y++){
        for(int x = 0; x < MAP_WIDTH; x++){
            if ((y == 0 || y == MAP_HEIGHT - 1) || (x == 0 || x == MAP_WIDTH - 1)){
                game_state->map[y][x] = WALL;
            }
            else {
                game_state->map[y][x] = VOID;
            }
        }
    }
}

void print_map(t_game_state *game_state){
    int player_pos_x = (int)floor(game_state->player->pos_x);
    int player_pos_y = (int)floor(game_state->player->pos_y);
    for(int y = 0; y < MAP_HEIGHT; y++){
        for(int x = 0; x < MAP_WIDTH; x++){
            if (game_state->map[y][x] == 1){
                printf("#");
            }
            else if (x == player_pos_x && y == player_pos_y )
            {
                if (fabs(game_state->player->dir_x) > fabs(game_state->player->dir_y))
                {
                    if (game_state->player->dir_x < 0 ){
                        printf("<");
                    }
                    else {
                        printf(">");
                    }
                }
                else
                {
                    if (game_state->player->dir_y < 0){
                        printf("^");
                    }
                    else
                    {
                        printf("V");
                    }
                }
            }
            else
            {
                printf(".");
            }
        }
        printf("\n");
    }

    printf("position = (%.2f, %.2f)\n", game_state->player->pos_x, game_state->player->pos_y);
    printf("direction = (%.2f, %.2f)\n", game_state->player->dir_x, game_state->player->dir_y);
}
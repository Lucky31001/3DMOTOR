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
    // "." for void
    // "#" for a wall
    for(int y = 0; y < MAP_HEIGHT; y++){
        for(int x = 0; x < MAP_WIDTH; x++){
            if (game_state->map[y][x] == 1){
                printf("#");
            }
            else if (x == player_pos_x && y == player_pos_y )
            {
                printf(">");
            }
            else
            {
                printf(".");
            }
        }
        printf("\n");
    }

    printf("position = (%.2f, %.2f)\ndirection = east\n", game_state->player->pos_x, game_state->player->pos_y);
}
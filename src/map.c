#include<stdio.h>
#include"utils.h"

void build_map(t_game_state *game_state){
    // 0 for void
    // 1 for wall
    for(int y = 0; y < MAP_H; y++){
        for(int x = 0; x < MAP_W; x++){
            if ((y == 0 || y == MAP_H-1) || (x == 0 || x == MAP_W-1)){
                game_state->map[y][x] = 1;
            }
            else {
                game_state->map[y][x] = 0;
            }
        }
    }
}

void print_map(t_game_state *game_state){
    // "." for void
    // "#" for a wall
    for(int y = 0; y < MAP_H; y++){
        for(int x = 0; x < MAP_W; x++){
            if (game_state->map[y][x] == 1){
                printf("#");
            }
            else {
                printf(".");
            }

            if (x == MAP_W-1){
                printf("\n");
            }
        }
    }
}
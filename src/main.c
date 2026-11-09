#include<stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include"utils.h"
#include"map.h"
#include"player.h"

int main(void){
    t_player player;
    t_game_state game_state = { .player = &player};

    build_map(&game_state);
    build_player(&game_state);
    
    bool running = true;
    while (running)
    {
        printf("\033[2J");
        print_map(&game_state);

        char mouvement;
        printf("Choose a mouvement (%c%c%c%c to move, %c to exit) : ", UP, LEFT, DOWN, RIGHT, EXIT);
        scanf(" %c", &mouvement);
        if (toupper(mouvement) == EXIT){
            running = false;
        }
        move_player(&game_state, toupper(mouvement));
    }
    
    
    return 0;
}
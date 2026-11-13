#include<stdio.h>
#include<stdbool.h>
#include<ctype.h>
#include<math.h>
#include"utils.h"
#include"map.h"
#include"player.h"

int main(void){
    t_player player = { .dir_x = 0, .dir_y = 0};
    t_game_state game_state = { .player = &player};

    build_map(&game_state);
    build_player(&game_state);
    
    bool running = true;
    while (running)
    {
        printf("\033[2J");
        printf("\033[H");
        print_map(&game_state);

        char mouvement;
        printf("Choose a mouvement (%c%c%c%c to move, %c to exit) : ", UP, LEFT, DOWN, RIGHT, EXIT);
        scanf(" %c", &mouvement);
        if (toupper(mouvement) == EXIT){
            running = false;
        }
        switch (toupper(mouvement))
        {
        case UP:
            move_player(&game_state, MOVE);
            break;
        case LEFT:
            rotate_player(&game_state, -M_PI/100);
            break;
        case DOWN:
            move_player(&game_state, -MOVE);
            break;
        case RIGHT:
            rotate_player(&game_state, M_PI/100);
            break;
        default:
            break;
        }
    }
    
    
    return 0;
}
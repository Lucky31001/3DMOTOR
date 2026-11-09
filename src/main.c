// #include <stdbool.h>
#include"utils.h"
#include"map.h"
#include"player.h"

int main(void){
    t_player player;
    t_game_state game_state = { .player = &player};

    build_map(&game_state);
    build_player(&game_state);
    
    // bool running = true;
    
    print_map(&game_state);
    return 0;
}
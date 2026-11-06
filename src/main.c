#include"map.h"
#include"utils.h"

int main(){
    t_game_state game_state;
    build_map(game_state);
    print_map(game_state);
    return 0;
}
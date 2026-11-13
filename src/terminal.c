#include<unistd.h>
#include<stdbool.h>
#include <termios.h>
#include"utils.h"
#include"terminal.h"

void set_terminal(t_terminal *terminal){
    
    int success_get = tcgetattr(STDIN_FILENO, &terminal->origin);
    if (success_get != 0) {
        terminal->success = false;
        return;
    }

    terminal->custom = terminal->origin;
    terminal->custom.c_lflag = terminal->custom.c_lflag & ~(ECHO | ICANON);
    int success_set = tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal->custom);
    if (success_set != 0){
        terminal->success = false;
        return;
    }

    terminal->success = true;
}

void reset_terminal(t_terminal *terminal){
     if (terminal->success) tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal->origin);
}
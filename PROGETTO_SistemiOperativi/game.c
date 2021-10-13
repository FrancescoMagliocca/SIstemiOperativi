#define _POSIX_SOURCE
/*macro che controlla le condizioni di sistema*/
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "library/board.h"
#include "library/msg_manager.h"
#include "library/player.h"
#include "library/types.h"

/* EASY MODE */

#define SO_NUM_G 2
#define SO_NUM_P 10
#define SO_MAX_TIME 3
#define SO_BASE 60
#define SO_ALTEZZA 20
#define SO_FLAG_MIN 5
#define SO_FLAG_MAX 5
#define SO_ROUND_SCORE 10
#define SO_N_MOVES 20
#define SO_MIN_HOLD_NSEC 100000000


/* HARD MODE */
/*
#define SO_NUM_G 4
#define SO_NUM_P 400
#define SO_MAX_TIME 1
#define SO_BASE 120
#define SO_ALTEZZA 40
#define SO_FLAG_MIN 5
#define SO_FLAG_MAX 40
#define SO_ROUND_SCORE 200
#define SO_N_MOVES 200
#define SO_MIN_HOLD_NSEC 100000000
*/

unsigned int current_placing_player, current_round, total_playing_time, ready_players, conquered_flags, flag_count;
player_t player_list[SO_NUM_G];
time_t round_start_time;
int game_board_shm_id;
board_t* game_board;

void signal_handler(int signo);
void exec_round();
void handle_message(message_t* message);
void end_round();
void end_game();
void kill_process();
void start_again();

int main() {
    message_t message;/*struct*/
    printf("Welcome in the game!\n");
    current_round = total_playing_time = 0;
    game_board_shm_id = generate_board(SO_BASE, SO_ALTEZZA);
    game_board = get_board(game_board_shm_id);
    game_board->waiting_time = SO_MIN_HOLD_NSEC;
    printf("A %dx%d board has been generated!\n", SO_BASE, SO_ALTEZZA);
    signal(SIGALRM, signal_handler);/*manda questo segnale quando il tempo raggiunge lo 0*/
    spawn_players(player_list, game_board_shm_id, SO_NUM_G, SO_NUM_P, SO_N_MOVES);
    if(game_board->coordinator_pid == getpid()) {
        printf("%d players has been created!\n", SO_NUM_G);
        while(1) {
            message = receive_message(game_board->coordinator_mq_id);/* sempre in attesa di messaggio*/
            handle_message(&message);/*questo è il gioco*/
        }
    }
    return 0;
}

void handle_message(message_t* message) {
    unsigned int index;
    switch(message->message_type) {
        case 1: {                                    /*aumenta i giocatori pronti*/
            ready_players++;
            if (ready_players == SO_NUM_G){/* quando i giocatori sono tutti pronti piazziamo le pedine*/
                current_placing_player = ready_players = 0;
                allow_pawn_placing(&player_list[current_placing_player]);
            }
        } break;
        case 3: {
            current_placing_player++; /*il giocatore successivo piazza la pedina*/
            if(current_placing_player == SO_NUM_G) {/*riparte dal primo*/
                current_placing_player = 0;
            }
            allow_pawn_placing(&player_list[current_placing_player]);
        } break;
        case 4: {
            exec_round();
        } break;
        case 6: {
            ready_players++;
            if (ready_players == SO_NUM_G){
                ready_players = 0;
                round_start_time = time(NULL);/*You can pass in a pointer to a time_t object that time will fill up with the current time (and the return value is the same one that you pointed to). If you pass in NULL, it just ignores it and merely returns a new time_t object that represents the current time.*/
                alarm(SO_MAX_TIME);/*interrompi al tempo massimo*/
                broadcast_signal_to_players(player_list, SO_NUM_G, 7);/*inizio*/
            }
        } break;
        case 9:{
            printf("Flag conquered by %c!\n", message->player_pseudo_name);
            broadcast_signal_to_players(player_list, SO_NUM_G, 9);
            conquered_flags++;
            if ( conquered_flags == flag_count ){
                printf("Every flag has been conquered! End of the round\n");
                end_round();
                print_status(game_board, player_list, SO_NUM_G);
                start_again();
            }
        } break;
        case 10:{/* diminuisce il suo numero di mosse*/
            index = get_player_index(player_list, SO_NUM_G, message->player_pseudo_name);
            if (index != -1){
                player_list[index].available_moves--;
            }
        } break;
    }
}

void exec_round(){
    printf("Starting a new round!\n");
    conquered_flags = 0;
    current_round++;
    game_board->round_in_progress = 1;
    flag_count = spawn_flags(game_board, SO_FLAG_MIN, SO_FLAG_MAX, SO_ROUND_SCORE);
    printf("%d flags has been placed!\n", flag_count);
    print_board(game_board);
    printf("Game start!\n");
    broadcast_signal_to_players(player_list, SO_NUM_G, 5);
}

void signal_handler(int signo){
    if (signo == SIGALRM){
        end_game();
    }
}

void end_round(){
    game_board->round_in_progress = 0;
    update_players_score(game_board, player_list, SO_NUM_G, 1);/*player*/
    total_playing_time += time(NULL) - round_start_time;
    alarm(0);
}

void end_game(){
    end_round();
    kill_process();
    printf("Played round: %d\n", current_round);
    printf("GAME OVER! Time out\n");
    print_status(game_board, player_list, SO_NUM_G);
    destroy_board(game_board);
    printf("All resources have been deallocated! The game ends here. Thank you for playing and see you next time!\n");
    exit(0);
}

void kill_process() {
    unsigned int i;
    broadcast_signal_to_players(player_list, SO_NUM_G, 11);
    for(i = 0; i < SO_NUM_G; i++) {
        close_message_queue(player_list[i].mq_id);
    }
}

void start_again() {
    unsigned int i, moves;
    moves = SO_NUM_P * SO_N_MOVES;
    for(i = 0; i < SO_NUM_G; i++) {
        player_list[i].available_moves = moves;
    }
    remove_flags(game_board);
    broadcast_signal_to_players(player_list, SO_NUM_G, 12);
    exec_round();
}
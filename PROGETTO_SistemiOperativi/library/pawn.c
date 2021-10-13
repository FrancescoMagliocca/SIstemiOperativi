#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pawn.h"
#include "board.h"
#include "msg_manager.h"
#include "types.h"

coords_t get_next_position(board_t* game_board, coords_t* current_position) {
    coords_t position;
    int orientation; /*Used to decide the direction to take during movement*/
    boolean valid;
    do{
        valid = 1;
        orientation = (int)lrand48() % 5;/*viene scelta una orientazione da 1 a 4*/
        switch(orientation) {
            case 1: {/*verso il basso*/
                if(current_position->y == 0) {
                    valid = 0;
                } else {
                    position.x = current_position->x;
                    position.y = current_position->y - 1;
                }
            } break;
            case 2: {/*verso dx*/
                if (current_position->x == game_board->width) {
                    valid = 0;
                } else {
                    position.x = current_position->x + 1;
                    position.y = current_position->y;
                }
            } break;
            case 3: {/*verso l'alto*/
                if(current_position->y == game_board->height) {
                    valid = 0;
                } else {
                    position.x = current_position->x;
                    position.y = current_position->y + 1;
                }
            } break;
            case 4: {/*sx*/
                if(current_position->x == 0) {
                    valid = 0;
                } else {
                    position.x = current_position->x - 1;
                    position.y = current_position->y;
                }
            } break;
            default: {
                valid = 0;
            } break;
        }
    } while(valid == 0);
    position.index = compute_index(game_board, &position);
    return position;
}

void signal_achievement(board_t* game_board, char player_pseudo_name) {/*manda un messaggio al master di conquista*/
    message_t message;
    message.message_type = 9;
    message.player_pseudo_name = player_pseudo_name;
    send_message(game_board->coordinator_mq_id, &message);
}

void notify_movement(board_t* game_board, char player_pseudo_name) {
    message_t message;

    if(game_board->round_in_progress == 1) {/* al master*/
        message.message_type = 10;
        message.player_pseudo_name = player_pseudo_name;
        send_message(game_board->coordinator_mq_id, &message);
    }
}

pawn_t spawn_pawn(board_t* game_board, char player_pseudo_name, int game_board_shm_id, unsigned int max_moves) {
    pid_t pawn_pid;
    int pawn_mq_id;
    pawn_t pawn;
    pawn_mq_id = generate_message_queue();
    pawn_pid = fork();
    if(pawn_pid == -1) {
        printf("Cannot fork process, aborting.\n");
        exit(5);
    }else if(pawn_pid == 0) {/*fork di pedina*/
        unsigned int available_moves;
        boolean flag_conq;
        board_t* local_board;
        coords_t next_pos;
        message_t message;
        coords_t position;
        available_moves = max_moves;
        local_board = get_board(game_board_shm_id);
        position = get_random_position(game_board, 0);
        place_pawn(local_board, &position, player_pseudo_name);/* sta in board*/
        while(1) {
            message = receive_message(pawn_mq_id);
            switch(message.message_type) {
                case 8: {
                    while(available_moves > 0) {
                        if(game_board->round_in_progress != 1) {
                            break;
                        }
                        next_pos = get_next_position(local_board, &position);
                        flag_conq = move_pawn(local_board, &position, &next_pos, player_pseudo_name);
                        position = next_pos;
                        available_moves--;
                        notify_movement(local_board, player_pseudo_name);
                        if(flag_conq == 1) {
                            available_moves = 0;
                            signal_achievement(local_board, player_pseudo_name);/*bandierina conquista*/
                        }
                    }
                }break;
                case 11: {
                    exit(0);
                }
                case 12: {
                    available_moves = max_moves;
                }break;
            }
        }
    }else{
        pawn.mq_id = pawn_mq_id;
        pawn.pid = pawn_pid;
    }
    return pawn;
}

void broadcast_message_to_pawns(pawn_t* pawn_list, unsigned int pawn_count, message_t* message) {
    unsigned int i;
    for(i = 0; i < pawn_count; i++) {
        send_message(pawn_list[i].mq_id, message);
    }
}

void broadcast_signal_to_pawns(pawn_t* pawn_list, unsigned int pawn_count, unsigned short type) {
    message_t message;
    unsigned int i;
    message.message_type = type;
    message.player_pseudo_name = 0;
    for(i = 0; i < pawn_count; i++) {
        send_message(pawn_list[i].mq_id, &message);
    }
}
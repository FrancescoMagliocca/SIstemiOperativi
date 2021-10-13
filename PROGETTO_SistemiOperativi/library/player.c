#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "player.h"
#include "board.h"
#include "pawn.h"
#include "msg_manager.h"
#include "types.h"

void send_signal_message_to_master(board_t* game_board, unsigned short type) {
    message_t message;
    message.message_type = type;
    message.player_pseudo_name = 0;/*manda i messaggi*/
    send_message(game_board->coordinator_mq_id, &message);
}

void end_placement(board_t* game_board, boolean all_placed) {
    unsigned short message_type;
    message_type = all_placed == 1 ? 4 : 3;
    send_signal_message_to_master(game_board, message_type);
}

void ready_up(board_t* game_board) {
    send_signal_message_to_master(game_board, 1);
}

void organization_completed(board_t* game_board) {
    send_signal_message_to_master(game_board, 6);
}

void destroy_pawns(pawn_t* pawn_list, unsigned int pawn_count) {
    unsigned int i;
    broadcast_signal_to_pawns(pawn_list, pawn_count, 11);
    for (i=0;i<pawn_count;i++) {
        close_message_queue(pawn_list[i].mq_id);
    }
}

void allow_pawn_placing(player_t* player) {
    message_t message;
    message.message_type = 2;
    message.player_pseudo_name = 0;
    send_message(player->mq_id, &message);
}

void spawn_players(player_t* player_list, int game_board_shm_id, unsigned int player_count, int pawn_count, unsigned int max_pawn_moves) {
    pid_t player_pid;
    char pseudo_name;
    int player_mq_id;
    unsigned int i;
    for(i = 0; i < player_count; i++) {
        pseudo_name = i + 65;
        player_mq_id = generate_message_queue();
        player_pid = fork();
        if(player_pid == -1) {
            printf("Impossible to fork process.\n");
            exit(3);
        }else if(player_pid == 0) {
            pawn_t pawn_list[pawn_count];
            board_t* game_board;
            int remaining_pawns;
            message_t message;
            printf("Player %d (%c) has joined the game.\n", i + 1, pseudo_name);
            remaining_pawns = pawn_count - 1;
            game_board = get_board(game_board_shm_id);
            ready_up(game_board);/*manda il messaggio al game che gioxatore x è pronto*/
            while(1) {
                message = receive_message(player_mq_id);
                switch(message.message_type) {
                    case 2: {
                        if (remaining_pawns >= 0) {/* se ci sono ancora pedine viene mandato un messaggio di tipo 0*/
                            pawn_list[remaining_pawns] = spawn_pawn(game_board, pseudo_name, game_board_shm_id, max_pawn_moves);
                            remaining_pawns--;
                            end_placement(game_board, 0);
                        } else {
                            end_placement(game_board, 1);
                        }
                    }break;
                    case 5: {
                        organization_completed(game_board);
                    }break;
                    case 7: {
                        broadcast_signal_to_pawns(pawn_list, pawn_count, 8);
                    }break;
                    case 9: {
                        broadcast_signal_to_pawns(pawn_list, pawn_count, 9);
                    }break;
                    case 11:{
                        destroy_pawns(pawn_list, pawn_count);
                        exit(0);
                    }
                    case 12: {
                        broadcast_signal_to_pawns(pawn_list, pawn_count, 12);
                    }break;
                }
            }
        }else{/*settati i valori della struct*/
            player_list[i].mq_id = player_mq_id;
            player_list[i].pid = player_pid;
            player_list[i].pseudo_name = pseudo_name;
            player_list[i].available_moves = pawn_count * max_pawn_moves;
            player_list[i].total_moves = player_list[i].available_moves;
            player_list[i].total_score = player_list[i].global_score = 0;
        }
    }
}

void broadcast_message_to_players(player_t* player_list, unsigned int player_count, message_t* message) {
    unsigned int i;
    for(i = 0; i < player_count; i++) {
        send_message(player_list[i].mq_id, message);
    }
}

void broadcast_signal_to_players(player_t* player_list, unsigned int player_count, unsigned short type) {
    message_t message;
    unsigned int i;
    message.message_type = type;
    message.player_pseudo_name = 0;
    for(i = 0; i < player_count; i++) {
        send_message(player_list[i].mq_id, &message);
    }
}

unsigned int get_player_index(player_t* player_list, unsigned int player_count, char player_pseudo_name) {/* prende l'indice del giocatore*/
    unsigned int i, index;
    index = -1;
    i = 0;
    while(index == -1 && i<player_count) {
        if(player_list[i].pseudo_name == player_pseudo_name) {
            index = i;
        }
        i++;
    }
    return index;
}

unsigned int get_player_score(board_t* game_board, char player_pseudo_name) {
    unsigned int i, length, score;
    length = game_board->width * game_board->height;
    score = 0;
    for(i = 0; i < length; i++) {
        if(game_board->cells[i].player_pseudo_name == player_pseudo_name) { 
            score += game_board->cells[i].flag_score;
        }
    }
    return score;
}

void update_players_score(board_t* game_board, player_t* player_list, unsigned int player_count, boolean update_glob) {
    unsigned int i;
    for(i = 0; i < player_count; i++) {
        player_list[i].total_score = get_player_score(game_board, player_list[i].pseudo_name); /* aggiorna e prende il punteggio*/ 
    }
    if(update_glob == 1) {
        for(i = 0; i < player_count; i++) {
            player_list[i].global_score += player_list[i].total_score;
        }
    }
}
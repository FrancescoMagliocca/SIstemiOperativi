#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

#include "types.h"

void spawn_players(player_t* player_list, int game_board_shm_id, unsigned int player_count, int pawn_count, unsigned int max_pawn_moves);
void update_players_score(board_t* game_board, player_t* player_list, unsigned int player_count, boolean update_glob);
unsigned int get_player_index(player_t* player_list, unsigned int player_count, char player_pseudo_name);
void broadcast_message_to_players(player_t* player_list, unsigned int player_count, message_t* message);
void broadcast_signal_to_players(player_t* player_list, unsigned int player_count, unsigned short type);
unsigned int get_player_score(board_t* game_board, char player_pseudo_name);
void allow_pawn_placing(player_t* player);

#endif
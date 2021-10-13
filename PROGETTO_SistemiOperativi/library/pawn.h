#ifndef GAME_PAWN_H
#define GAME_PAWN_H

#include "types.h"

pawn_t spawn_pawn(board_t* game_board, char player_pseudo_name, int game_board_shm_id, unsigned int max_moves);
void broadcast_message_to_pawns(pawn_t* pawn_list, unsigned int pawn_count, message_t* message);
void broadcast_signal_to_pawns(pawn_t* pawn_list, unsigned int pawn_count, unsigned short type);

#endif
#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include "types.h"

boolean move_pawn(board_t* game_board, coords_t* old_position, coords_t* new_position, char player_pseudo_name);
unsigned int spawn_flags(board_t* game_board, unsigned int min, unsigned int max, unsigned int max_score);
unsigned int compute_index_from_params(board_t* game_board, unsigned int x, unsigned int y);
void print_status(board_t* game_board, player_t* player_list, unsigned int player_count);
void print_stats(board_t* game_board, player_t* player_list, unsigned int player_count);
boolean place_pawn(board_t* game_board, coords_t* position, char player_pseudo_name);
coords_t get_random_position(board_t* game_board, boolean allow_occupied_by_flags);
unsigned int compute_index(board_t* game_board, coords_t* coords);
int generate_board(int width, int height);
void destroy_board(board_t* game_board);
void remove_flags(board_t* game_board);
void print_board(board_t* game_board);
board_t* get_board(int shm_id);

#endif
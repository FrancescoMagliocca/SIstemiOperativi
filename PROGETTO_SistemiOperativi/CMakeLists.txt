cmake_minimum_required(VERSION 3.15)
project(game C)

set(CMAKE_C_STANDARD 90)

add_executable(game game.c library/types.h library/board.c library/board.h library/player.c library/player.h library/pawn.c library/pawn.h library/msg_manager.c library/msg_manager.h)
/*albero di esecuzione*/
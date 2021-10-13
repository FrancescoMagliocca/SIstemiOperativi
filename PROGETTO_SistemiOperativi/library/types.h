#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdlib.h>
#include <sys/types.h>
#include <semaphore.h>

/*Definition of boolean type*/
typedef unsigned short boolean;

typedef struct {
    unsigned int x;
    unsigned int y;
    unsigned int index;
} coords_t;

typedef struct {
    char player_pseudo_name;
    unsigned int flag_score;
    unsigned short occupant_type;
    pid_t occupant_pid;
    sem_t mutex;
} cell_t;

typedef struct {
    int width;
    int height;
    int coordinator_mq_id;
    long waiting_time;
    pid_t coordinator_pid;
    boolean round_in_progress;
    cell_t cells[];
} board_t;

typedef struct {
    int owner_mq_id;
    int mq_id;
    pid_t pid;
} pawn_t;

typedef struct {
    int mq_id;
    pid_t pid;
    char pseudo_name;
    unsigned int available_moves;
    unsigned int total_moves;
    unsigned int total_score;
    unsigned int global_score;
} player_t;

typedef struct {
    unsigned short message_type;
    char player_pseudo_name;
    char payload[128];
} message_t;

#endif
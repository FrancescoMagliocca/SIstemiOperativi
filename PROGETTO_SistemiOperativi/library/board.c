#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#if __APPLE__
    #include <limits.h>
#else
    #include <linux/limits.h>
#endif
#include "board.h"
#include "msg_manager.h"
#include "types.h"
#include "player.h"

key_t generate_shm_key(int id) {
    char current_path[PATH_MAX];/*prende il path della cartella*/
    key_t shm_key;
    getcwd(current_path, sizeof(current_path));/*acquisisce il path dalla directory e viene creato un buffer*/
    shm_key = ftok(current_path, id);
    return shm_key;
}

int generate_shared_memory_segment(size_t size, int id) {
    key_t shm_key;
    int shm_id;
    shm_key = generate_shm_key(id);
    shm_id = shmget(shm_key, size, IPC_CREAT | 0660);
    if (shm_id<0) {
        printf("Impossible to allocate the shared memory.\n");
        printf("Error: %s.\n", strerror(errno));
        exit(1);
    }
    return shm_id;
}
int allocate_board(int width, int height) {
    size_t size;
    int shm_id;
    size = sizeof(board_t) + (sizeof(cell_t) * height * width);
    shm_id = generate_shared_memory_segment(size, 1);
    return shm_id;
}

/*Compute array index from given coords struct*/
unsigned int compute_index(board_t* game_board, coords_t* coords) { /*prende la struct posizione e prende gli x e y trova l'indice nell'array*/
    return coords->x * game_board->height + coords->y;/*nel caso gli dovessimo passsare una struct*/
}

/*Compute array index from x and y values (row and column index of the ideal board)*/        /*gli passiamo gli x e y*/
unsigned int compute_index_from_params(board_t* game_board, unsigned int x, unsigned int y) {
    return x * game_board->height + y;
}

int generate_board(int width, int height) {/* crea scahhiera*/
    unsigned int x, y, index;
    board_t* game_board;
    int shm_id;
    shm_id = allocate_board(width, height);
    game_board = get_board(shm_id);
    game_board->width = width;
    game_board->height = height;
    game_board->coordinator_mq_id = generate_message_queue();/*creazione coda messaggi*/
    game_board->coordinator_pid = getpid();
    game_board->waiting_time = game_board->round_in_progress = 0;
    for(x = 0; x < width; x++) {
        for(y = 0; y < height; y++) {
            index = compute_index_from_params(game_board, x, y);
            game_board->cells[index].flag_score = 0;
            game_board->cells[index].occupant_type = 0;
            game_board->cells[index].player_pseudo_name = 0;
            sem_init(&game_board->cells[index].mutex, 0, 1);/*crezione semaforo,no accessibile a tutti */
            /*il primo valore è il riferimento,il secondo che i threa del processo possono utilizzarlo,l'ultimo è il valore del sem*/
        }
    }
    return shm_id;
}
board_t* get_board(int shm_id) {/*fa l'attach*/
    board_t* game_board;
    void* shm_ptr;
    shm_ptr = shmat(shm_id, NULL, 0);
    if((int)shm_ptr == -1) {
        printf("Impossible to attach the shared memory.\n");
        printf("Error: %s.\n", strerror(errno));
        exit(2);
    }
    game_board = (board_t *)shm_ptr;
    return game_board;
}

coords_t get_random_position(board_t* game_board, boolean allow_occupied_by_flags) {
    unsigned short min_occ, current_occ;/* viene fatto solo una volta per bandierina*/
    pid_t current_pid;
    coords_t coords;
    min_occ = allow_occupied_by_flags == 1 ? 1 : 0;
    current_pid = getpid();
    srand(current_pid);
    do {
        coords.x = (int)lrand48() % game_board->width;
        coords.y = (int)lrand48() % game_board->height;
        coords.index = compute_index(game_board, &coords);
        current_occ = game_board->cells[coords.index].occupant_type;
    } while(current_occ > min_occ);
    return coords;
}

boolean place_pawn(board_t* game_board, coords_t* position, char player_pseudo_name) {
    boolean flag_conq;
    flag_conq = 0;
    sem_wait(&game_board->cells[position->index].mutex);
    if(game_board->cells[position->index].occupant_type == 0) {
        game_board->cells[position->index].occupant_type = 2;
        game_board->cells[position->index].player_pseudo_name = player_pseudo_name;
    }else if(game_board->cells[position->index].occupant_type == 1) {
        game_board->cells[position->index].occupant_type = 2;
        game_board->cells[position->index].player_pseudo_name = player_pseudo_name;
        flag_conq = 1;
    }
    sem_post(&game_board->cells[position->index].mutex);
    return flag_conq;
}

boolean is_allowed_position(board_t* game_board, coords_t* position) {/* la cella è vuota o ce una bandierina*/
    boolean allowed;
    sem_wait(&game_board->cells[position->index].mutex);
    allowed = game_board->cells[position->index].occupant_type <= 1 ? 1 : 0;
    sem_post(&game_board->cells[position->index].mutex);
    return allowed;
}

boolean move_pawn(board_t* game_board, coords_t* old_position, coords_t* new_position, char player_pseudo_name) {
    boolean flag_conq;
    struct timespec wait;
    flag_conq = 0;
    wait.tv_sec = 0;/* wait di time*/
    wait.tv_nsec = game_board->waiting_time;
    if(is_allowed_position(game_board, new_position) == 1) {
        sem_wait(&game_board->cells[old_position->index].mutex);
        game_board->cells[old_position->index].occupant_type = 0;
        game_board->cells[old_position->index].player_pseudo_name = 0;
        sem_post(&game_board->cells[old_position->index].mutex);
        flag_conq = place_pawn(game_board, new_position, player_pseudo_name);
    }
    nanosleep(&wait, NULL);
    return flag_conq;
}

unsigned int spawn_flags(board_t* game_board, unsigned int min, unsigned int max, unsigned int max_score) {
    unsigned int i, flag_count, n, score, r, res;
    coords_t position;
    flag_count = rand() % (max-min+1) + min; 
    r = max_score % flag_count;
    n = flag_count;
    res = r;
    /*Flag score assignment*/
    for(i = 0; i < flag_count; i++) {
        if(r == 0) { 
            score = max_score / n;
        } else {
            score= max_score / n + res;
            res = 0;
        }
        position = get_random_position(game_board, 0);
        game_board->cells[position.index].occupant_type = 1;
        game_board->cells[position.index].player_pseudo_name = 0;
        game_board->cells[position.index].flag_score = score;
    }
    return flag_count;
}

void print_board(board_t* game_board) {
    unsigned int x, y, index;
    for(y = 0; y < game_board->height; y++) {
        for(x = 0; x < game_board->width; x++) {
            index = compute_index_from_params(game_board, x, y);
            switch(game_board->cells[index].occupant_type) {
                case 0: {
                    printf("-"); /*'-' represents an empty cell*/
                } break;
                case 1: {
                    if(game_board->cells[index].player_pseudo_name > 0) {
                        printf("%c", game_board->cells[index].player_pseudo_name);
                    } else {
                    	
                        printf("f"); /*'f' represents a positition occupied by a flag*/
                    }
                } break;
                case 2: {
                    printf("%c", game_board->cells[index].player_pseudo_name);
                } break;
            }
        }
    }
    printf("\n");
}

void print_stats(board_t* game_board, player_t* player_list, unsigned int player_count) {
    unsigned int x, y, i, index, player_index;
    unsigned int scores[player_count];
    unsigned int max;
    unsigned int winner;
    for(y = 0; y < game_board->height; y++) {
        for(x = 0; x < game_board->width; x++) {
            index = compute_index_from_params(game_board, x, y);
            if(game_board->cells[index].player_pseudo_name != 0) {
                player_index = get_player_index(player_list, player_count, game_board->cells[index].player_pseudo_name);
                if(player_index != -1) {
                    scores[player_index] += game_board->cells[index].flag_score;
                }
            }
        }
    }
    printf("Round stats: \n");
    max = 0;
    winner = 0;
    for(i = 0; i < player_count; i++) {
        printf("Player %c:\n", player_list[i].pseudo_name);
        printf("\tScore: %d.\n", player_list[i].total_score);
        if(player_list[i].total_score > max) {
        	max = player_list[i].total_score;
        	winner = i;
        }
    }
    printf("\n");
    printf("The winner is: %c:\n", player_list[winner].pseudo_name);
    printf("\n");
}

void print_status(board_t* game_board, player_t* player_list, unsigned int player_count) {
    print_board(game_board);
    print_stats(game_board, player_list, player_count);
}

void destroy_board(board_t* game_board) {
    unsigned int length, i;
    length = game_board->width * game_board->height;
    for (i=0;i<length;i++) {
        if ( sem_destroy(&game_board->cells[i].mutex) == -1 ) {
            printf("Impossible to delete board.\n");
            printf("Error: %s.\n", strerror(errno));
            exit(5);
        }
    }
    close_message_queue(game_board->coordinator_mq_id);
}

void remove_flags(board_t* game_board) {
    unsigned int length, i;

    length = game_board->width * game_board->height;
    for (i=0;i<length;i++) {
        if ( game_board->cells[i].occupant_type == 1 ) {
            game_board->cells[i].occupant_type = 0;
        }
      
        game_board->cells[i].flag_score = 0;
    }
}
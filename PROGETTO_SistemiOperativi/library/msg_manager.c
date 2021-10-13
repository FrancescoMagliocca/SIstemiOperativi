#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include "types.h"
#include "msg_manager.h"

int generate_message_queue() {
    int message_queue_id;
    message_queue_id = msgget(IPC_PRIVATE, IPC_EXCL | IPC_CREAT | 0600);
    if (message_queue_id == -1) {
        printf("Impossible to initialize the message queue.\n");
        printf("Reported error: %s.\n", strerror(errno));
        exit(2);
    }
    return message_queue_id;
}

void send_message(int mq_id, message_t* msg) {
    size_t size;
    int result;
    size = strlen(msg->payload) + 1;
    result = msgsnd(mq_id, msg, size, 0);
    if(result == -1 && errno != EEXIST) {
        printf("Impossible to send message.\n", msg->message_type);
        printf("Reported error: %s.\n", strerror(errno));
        exit(4);
    }
}

message_t receive_message(int mq_id) {
    message_t msg;
    int result;
    result = msgrcv(mq_id, &msg, 8196, 0, 0);
    if(result == -1){}
    return msg;
}

void close_message_queue(int mq_id) {
    if(msgctl(mq_id, IPC_RMID, NULL) == -1) {
        printf("Impossible to close message queue.\n");
        printf("Reported error: %s.\n", strerror(errno));
        exit(5);
    }
}
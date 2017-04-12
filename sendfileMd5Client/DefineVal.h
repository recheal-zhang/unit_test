/*
 * Copyright (C) riozhang
 * Copyright (C) tencent, Inc.
 * */

#ifndef _DEFINEVAL_H_
#define _DEFINEVAL_H_

#include <sys/epoll.h>

#define INVALID_SOCKFD_VALUE 0

#define TIMEOUT 500



struct clientMsg{
    int seqId;
    int clientAcceptFd;
    int cost;
    //TODO: should extend
};

struct serverProcessMsg{
    int serverConnectFd;
    int count;
};

struct threadMsg{
    int epollfd;
//    char *buf;
    struct clientMsg cliMsg;
    struct serverProcessMsg svrProMsg;

    bool isClientQuery;
    struct epoll_event event;
};


#endif /*_DEFINEVAL_H_*/



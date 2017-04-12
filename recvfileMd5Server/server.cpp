#include <iostream>
#include <string>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define IPADRESS "127.0.0.1"
#define PORT 124
#define MAXSIZE 1024
#define LISTENQ 5
#define FDSIZE 1000
#define EPOLLEVENTS 100

long long queryNum = 0;
pthread_mutex_t queryNumMutexLock = PTHREAD_MUTEX_INITIALIZER;
struct timeval startval;
struct timeval endval;
int cost = 1;


//save file
int gFileFd[EPOLLEVENTS] = {0};

void *show_msg_thread(void *){
    std::cout << "come in" << std::endl;
    while(true){
//       gettimeofday(&endval, NULL);
//       long long cost = (endval.tv_sec - startval.tv_sec) * 1000000 + (endval.tv_usec - startval.tv_usec);
       pthread_mutex_lock(&queryNumMutexLock);
       int qps = static_cast<int>(queryNum / (2));
       queryNum = 0;
       pthread_mutex_unlock(&queryNumMutexLock);
       std::cout << "qps = " << qps << std::endl;
        sleep(2);
        if(cost == 1){cost = 2;}
        else{cost += 2;}

    }
}


void add_query_num(){
    pthread_mutex_lock(&queryNumMutexLock);
    queryNum++;
    pthread_mutex_unlock(&queryNumMutexLock);
}

//function declaration
int socket_bind(const char *ip, int port);

void do_epoll(int listenfd);

void sherror(const char *err);

void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char *buf);

void handle_accept(int epollfd, int listenfd);

void do_read(int epollfd, int fd, char *buf);

void do_write(int epollfd, int fd, char *buf);

void add_event(int epollfd, int fd, int state);

void modify_event(int epollfd, int fd, int state);

void delete_event(int epollfd, int fd, int state);

int main(int argc, char **argv){
    int listenfd;
    pthread_t showMsgThreadId;
    pthread_create(&showMsgThreadId, NULL, show_msg_thread, NULL);
    listenfd = socket_bind(IPADRESS, PORT);
    listen(listenfd, LISTENQ);
    gettimeofday(&startval, NULL);
    do_epoll(listenfd);
    return 0;
}

int socket_bind(const char *ip, int port){
    int listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << listenfd << std::endl;
    if(listenfd == -1){
        sherror("socket error");
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        sherror("bind error");
    }
    return listenfd;
}

void do_epoll(int listenfd){
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    char buf[MAXSIZE];
    memset(buf, 0, MAXSIZE);

    epollfd = epoll_create(FDSIZE);

    add_event(epollfd, listenfd, EPOLLIN);

    while(true){
        ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        handle_events(epollfd, events, ret, listenfd, buf);
    }

    close(epollfd);
}


void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char *buf){
    int i;
    int fd;

    for(i = 0; i < num; i++){
        fd = events[i].data.fd;

        if((fd == listenfd) && (events[i].events & EPOLLIN)){
            add_query_num();
            handle_accept(epollfd, listenfd);
        }
        else if(events[i].events & EPOLLIN){
            //TODO:multithread
            add_query_num();
            do_read(epollfd, fd, buf);
        }
        else if(events[i].events & EPOLLOUT){
            //TODO:multithread
            do_write(epollfd, fd, buf);
        }
    }
}

void handle_accept(int epollfd, int listenfd){
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    clifd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    if(clifd == -1){
        std::cout << "accept error" << std::endl;
    }
    //TODO:get client
    else{
        //create file
        std::stringstream ss;
        ss << clifd;
        std::string clifdStr;
        ss >> clifdStr;
        std::string filename = clifdStr + ".temp";

        add_event(epollfd, clifd, EPOLLIN);

        gFileFd[clifd] = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        //add_event(epollfd, gFileFd[clifd], EPOLLOUT);
    }
}

void do_read(int epollfd, int fd, char *buf){
    int nread;
    nread = read(fd, buf, MAXSIZE);
    if(nread == -1){
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
        std::cout << "read error" << std::endl;
    }
    else if(nread == 0){
        std::cout << "client close" << std::endl;
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
    }
    else{
        //TODO:read msg
        //add_event(epollfd, gFileFd[fd], EPOLLOUT);
        write(gFileFd[fd], buf, strlen(buf));
        //TODO:modify event
        //modify_event(epollfd, fd, EPOLLOUT);
    }
}

void do_write(int epollfd, int fd, char *buf){
    int nwrite;
    nwrite = write(fd, buf, strlen(buf));
    if(nwrite ==-1){
        std::cout << "write error" << std::endl;
        close(fd);
        delete_event(epollfd, fd, EPOLLOUT);
    }
    else{
        //modify_event(epollfd, fd, EPOLLIN);
    }
    memset(buf, 0, MAXSIZE);
}

void add_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void delete_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

void modify_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void sherror(const char *err){
#ifdef DEBUG
    std::cout << std::string(err) << std::endl;
#endif
    exit(-1);
}

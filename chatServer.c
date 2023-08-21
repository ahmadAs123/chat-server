#include <signal.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include "chatServer.h"
#include <stdlib.h>
#include <malloc.h>
#include <sys/ioctl.h>
static int end_server = 0;
void intHandler(int SIG_INT) {// when we must go out of the loop it will be 1
    end_server = 1;
}

int main (int argc, char *argv[])
{
    char  res[BUFFER_SIZE]; // the buffer thaat we put in the message when we read from socket
    int sockfd;
    char*mpet;
    int rc;
    int on = 1;
    signal(SIGINT, intHandler);// signal whem the breaking whule is now
    if(argc>2||argc<2){// if we inter mor or less than 2 elemnts
        printf("Usage: chatServer <port>\n");
        exit(1);
    }
    if(atoi(argv[1])<0)// if the port wasnt a number or negative
    {
        printf("Usage: chatServer <port>\n");
        exit(EXIT_FAILURE);
    }
    if(atoi(argv[1])>65536){
        printf("%d",atoi(argv[1]));
        printf("Usage: chatServe1r <port>\n");
        exit(EXIT_FAILURE);
    }
    conn_pool_t* pool = malloc(sizeof(conn_pool_t));// creating the pool <---------------------------------------------------------------------------------------------------------------------------
    if(pool==NULL){// malloc didnt succes

        return -1;
    }
    init_pool(pool);// intilizing the pool
     sockfd = socket(AF_INET, SOCK_STREAM, 0);// making socket
    if (sockfd < 0) {// the soccket fauld
        perror("Error while creating this socket!!!");
        exit(1);
    }

     rc = ioctl(sockfd, FIONBIO, (char *)&on);// nonblocking maker
    if (rc < 0) {// this faild
        perror("Error the nonblocking!");
        exit(1);
    }
    pool->maxfd = sockfd;
    struct sockaddr_in server_addr;
   memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));// with the port that the client inter

    int c = bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));// binding proccess
    if (c < 0) {// faild
        perror("error binding");
        exit(1);
    }


  int r=listen(sockfd, 5);// listening proccess
    if (r < 0) {// listening faild
        perror("Error  listening!!");
        exit(1);
    }

    FD_SET(sockfd,&pool->read_set);// intilizing the fd sets with zero and set the main socet for reading
    FD_ZERO(&pool->ready_read_set);
    FD_ZERO(&pool->ready_write_set);
    FD_ZERO(&pool->write_set);
    do // the loop
    {
        printf("Waiting on select()...\nMaxFd %d\n", pool->maxfd);
        pool->ready_write_set=pool->write_set;// copy master to the working
       pool->ready_read_set=pool->read_set;// copy master to the working
       int resu= select(pool->maxfd + 1, &pool->ready_read_set, &pool->ready_write_set,NULL,NULL);// selecting fd
       if(resu<0){// the select faild
           return -1;
       }
  int j;int gh;
  struct sockaddr_in srv;
  int f;int fd;
  struct sockaddr_in client;
  int new;
  int cli_len = sizeof(client);
            for(j=0;j<=pool->maxfd;j++) {// loop on all the fd
                if (FD_ISSET(j, &pool->ready_read_set)) {// check if ready to read
                    if (j == sockfd) {// if this the listening socket
                        new = accept(j, (struct sockaddr *) &client, &cli_len);//accepting one
                        if (new < 0) {// the accept faild
                            perror("accept faild!!!");
                            continue;
                        }
                        add_conn(new, pool);// add to the list of connection
                        printf("New incoming connection on sd %d\n", new);
                    } else {// not the main socet
                        int result=read(j, res, 4096);// read the content of the socket to buffer
                        printf("Descriptor %d is readable\n", j);
                        if (result==0) {
                            printf("removing connection with sd %d \n", j);// if the conneection closed
                            remove_conn(j, pool);// remove connection from the list
                            printf("Connection closed for sd %d\n", j);
                        }
                        printf("%d bytes received from sd %d\n", result, j);// adding the message to the all the fd only him we dont add
                        add_msg(j, res, result, pool);
                    }
                }
               if (FD_ISSET(j, &pool->ready_write_set)) {//if the socket was for writing
                   int y= write_to_client(j, pool);// write to the client
                if(y==-1){// we didnt success writing it
                    continue;
                }
               }
           }
        FD_ZERO(&pool->ready_read_set);// rezero the fd sets
        FD_ZERO(&pool->ready_write_set);// rezero the fd sets
    }
    while (end_server == 0);
    free(pool);// free the pool
    return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------functions

int init_pool(conn_pool_t* pool) {// initilizing  the pool with zeros and NULLs
    pool->nready=0;
    pool->maxfd=0;
    pool->nr_conns=0;
    FD_ZERO(&pool->write_set);// set ready to write
    pool->conn_head=NULL;
    FD_ZERO(&pool->read_set);
    FD_ZERO(&pool->ready_write_set);
    FD_ZERO(&pool->ready_read_set);
    return 0;
}

int add_conn(int sd, conn_pool_t* pool) {// add conection with sd to the list in this  pool and intilize all the
    if (pool==NULL) {// the pool was NULL
        return -1;
    }
    if (pool->maxfd<sd) {// if the sd was bigger than
        pool->maxfd=sd;
    }

    if (sd<0) {// sd is smaller than 0
        return -1;
    }

    conn_t *new=(conn_t *) malloc(sizeof(conn_t));// allocating the conn<--------------------------------------------------------------------------------------------------------------------------------------------------
    if (new == NULL) {//allocatin g was faild!!!
        return -1;
    }

    if (pool->conn_head!=NULL) {// the connection list isnt empty then add to the list
        pool->conn_head->prev=new;
        new->next=pool->conn_head;
        new->prev=NULL;
        pool->conn_head=new;
        FD_SET(sd,&pool->read_set);// set to read
        new->fd=sd;
        pool->nr_conns+=1;
        return 0;
    }
    else {// the list was empty
        new->fd=sd;
        new->next=NULL;
        pool->conn_head=new;
        new->prev=NULL;
        FD_SET(sd, &pool->read_set);// set to read
        pool->nr_conns+=1;
        return 0;
    }
}

int remove_conn(int sd, conn_pool_t* pool) {// removing a sd from conn list in this pool
    if (pool==NULL) {
        return -1;
    }
    if (sd<0) {
        return -1;
    }

    if (pool->maxfd<sd) {
        return -1;
    }
     struct conn*cur_con=pool->conn_head;
    if(cur_con->fd==sd&&pool->nr_conns==1){// it is the first of the list
        pool->nr_conns-=1;
        pool->conn_head=NULL;
        return 0;
    }

    else if(cur_con->fd==sd&&pool->nr_conns>1){// there is more than  1 anf dits the first head of the list
        pool->conn_head=pool->conn_head->next;
        pool->conn_head->prev=NULL;
        pool->nr_conns-=1;
        return 0;
    }

    else{// its in the middle of the list
      for(int i=0;i<pool->nr_conns;i++){
           if(cur_con->fd==sd){
               if(i==pool->nr_conns-1){
                  cur_con=cur_con->prev;
                   pool->nr_conns-=1;
                  cur_con->next=NULL;
                   FD_CLR(sd,&pool->read_set);
                   FD_CLR(sd,&pool->write_set);
                   FD_CLR(sd,&pool->ready_read_set);
                  return 0;
               }
               else {
                   struct conn * tmp=cur_con->next;
                   pool->nr_conns-=1;
                   cur_con=cur_con->prev;
                   cur_con->next=tmp;
                   FD_CLR(sd, & pool -> write_set);
                   FD_CLR(sd, & pool -> read_set);
                   FD_CLR(sd, & pool -> ready_read_set);
                   return 0;
               }
           }
       }
    }
    return -1;
}
//------------------------------------------------------------------------------
int add_msg(int sd,char* buffer,int len,conn_pool_t* pool) {// add message to the all sd 7ots m this sd
    if(sd<0)// the socet is negative
        return -1;
     if (len <0)// len is negative
         return -1;
     if(pool==NULL)// pool isnt intlized t=yet
         return -1;
    conn_t* tmp= pool->conn_head;// pointer on the head of the connection list
    while (tmp != NULL) {// loop on all connections
        if(tmp==NULL)
            break;
        if (tmp->fd != sd) {// check if this connection isnt have the same sd that we gwt
            msg_t* n =(msg_t*) malloc(sizeof(msg_t));// allocating msg<----------------------------------------------------------------------------------------------------------------------------------------------------
            if(n==NULL){
                continue;
                return -1;}
            n->message=(char*)malloc(len+1* sizeof(char));// allocating the meassage to pe saved<--------------------------------------------------------------------------------------------------------------------------
            if(n->message==NULL)
                return -1;
            n->size=len;// renew the length of the message
            strncpy( n->message, buffer,len);// copy the message to the n->message
            n->message[len+1]='\0';
            if(tmp->write_msg_head==NULL){// check if the list was empty
             tmp->write_msg_tail=n;
                tmp->write_msg_head=n;
                n->next=NULL;
                FD_SET(tmp->fd,&pool->write_set);// set to be ready to write
                n->prev=NULL;
                return 0;
            }
            else{// the list message isnt empty so add to the last
                tmp->write_msg_tail->next=n;
                n->prev=tmp->write_msg_tail;
                tmp->write_msg_tail=n;
                FD_SET(tmp->fd,&pool->write_set);// set to be ready to write
                return 0;
            }
        }
        tmp = tmp->next;
    }

}
//--------------------------------------------------------------------------
int write_to_client(int sd,conn_pool_t* pool) {
    int len;
    if(sd<0)
        return -1;
    if(pool==NULL)
        return -1;
    conn_t* con_h = pool->conn_head;// pointer on the head of connections
    while( con_h!= NULL) {// loop on all the coonection
        if (con_h->fd == sd) {// if this cobnnection have the same sd
            msg_t * a; //point on the first mesasage in th elist
            for(a=con_h->write_msg_head;a!=NULL; a=a->next){// loop on  the list of message isnt emnpty
                len=a->size;
                int re=write(sd,a->message,len);// wwrite the message to this socket
                if(re<0){// the write faild!!
                    continue;
                    return -1;
                }
            }
            con_h->write_msg_tail=NULL;
            FD_CLR(sd, &pool->write_set);//set message to be write
            con_h->write_msg_head=NULL;
            return 0;
        }
        con_h = con_h->next;
    }
    return -1;
}



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define NAME_SIZE 20


void * handle_clnt(void * arg);
void send_msg_all(char * msg, int len);
void * send_msg(void * arg);
void error_handling(char * msg);

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;


char name[NAME_SIZE]="[DEFAULT]";

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz, i;
	pthread_t snd_thread, rcv_thread;
	if(argc!=3) {
		printf("Usage : %s <port> <name>\n", argv[0]);
		exit(1);
	}
  	
  	sprintf(name,"[%s]",argv[2]);
	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET; 
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");


	while(1)
	{

				clnt_adr_sz=sizeof(clnt_adr);
				clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
				printf("connected client: %d \n",clnt_sock);

				pthread_mutex_lock(&mutx);
				clnt_socks[clnt_cnt++]=clnt_sock;
				pthread_mutex_unlock(&mutx);
			
				pthread_create(&rcv_thread, NULL, handle_clnt, (void*)&clnt_sock);
				pthread_create(&snd_thread, NULL, send_msg, (void*)&clnt_sock);
				pthread_detach(rcv_thread);
				pthread_detach(snd_thread);
				printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));

		
		
	}
	close(serv_sock);
	return 0;
}
void * send_msg(void * arg)
{
	int clnt_sock=*((int*)arg);
	char msg[BUF_SIZE];
	char name_msg[NAME_SIZE+BUF_SIZE];

	while(1)
	{
		fgets(msg, BUF_SIZE, stdin);
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n"))
		{
			return NULL;
		}
		sprintf(name_msg,"%s %s",name, msg);
		send_msg_all(name_msg, strlen(name_msg));
		fputs(name_msg,stdout);
	}

}
	
void * handle_clnt(void * arg)
{
	int clnt_sock=*((int*)arg);
	int str_len=0, i;
	char msg[BUF_SIZE];
	
	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0){
		send_msg_all(msg, str_len);
		msg[str_len]=0;
		fputs(msg, stdout);
	}
	
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)   // remove disconnected client
	{
		if(clnt_sock==clnt_socks[i])
		{
			while(i++<clnt_cnt-1)
				clnt_socks[i]=clnt_socks[i+1];
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}
void send_msg_all(char * msg, int len)   // send to all
{
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)
		write(i, msg, len);
	pthread_mutex_unlock(&mutx);
}
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
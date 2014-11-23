#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100

void* request_handler(void* arg);
void error_handling(char* message);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_size;
	char buf[BUF_SIZE];
	pthread_t t_id;	
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 20)==-1)
		error_handling("listen() error");

	while(1)
	{
		clnt_adr_size=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
		printf("Connection Request : %s:%d\n", 
			inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
		pthread_create(&t_id, NULL, request_handler, &clnt_sock);
		pthread_detach(t_id);
	}
	close(serv_sock);
	return 0;
}

void* request_handler(void *arg)
{
	int clnt_sock=*((int*)arg);
	char req_line[BUF_SIZE];
	char buf[BUF_SIZE];
	int rcvd, fd, bytes_read;
	char *root;
	
	char path[SMALL_BUF];

	// FILE* clnt_read;
	// FILE* clnt_write;
	char *req_word[3];
	char method[10];
	char ct[15];
	char file_name[30];


	root = getenv("PWD");
	memset((void*)req_line, (int)'\0',BUF_SIZE);

	rcvd=recv(clnt_sock, req_line, BUF_SIZE, 0);
	if(rcvd<0)
		fprintf(stderr, "recv() error\n");
	else if(rcvd==0)
		fprintf(stderr, "Client disconnected \n");
	else
	{
		// printf("req_line ::::\n %s\n", req_line);
		req_word[0] = strtok(req_line, " \t\n");
		if(strstr(req_word[0], "GET")!=NULL)
		{
			req_word[1] = strtok(NULL, " \t");
			req_word[2] = strtok(NULL, " \t\n");
			printf(" %s \n %s \n  %s\n", req_word[0],req_word[1],req_word[2]);
			if(strstr(req_word[2], "HTTP/")==NULL)
			{
				write(clnt_sock, "HTTP/1.0 400 Bad Request\n",25);
			}
			else
			{
				if(strncmp(req_word[1], "/\0", 2)==0)
				{
					req_word[1] = "/index.html";
				}
				strcpy(path,root);
				strcpy(&path[strlen(root)], req_word[1]);

				if((fd=open(path, O_RDONLY))!=-1)
				{
					write(clnt_sock, "HTTP/1.0 200 OK\n\n", 17);
					while((bytes_read=read(fd, buf, BUF_SIZE))>0)
						write(clnt_sock, buf, bytes_read);
				}
				else
				{
					write(clnt_sock,"HTTP/1.0 404 Not FOUND\n",23);
					printf("404 error\n");
				}
			}
		}
	} 
	shutdown(clnt_sock, SHUT_RDWR);
	close(clnt_sock);
	return NULL;
}
void error_handling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

#include <stdio.h> // for perror
#include <string.h> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <pthread.h> //for multithread
#include <stdlib.h> //for char * to int


/* global variable*/
pthread_mutex_t mutx;
int client[10];
int client_num =0;
bool option =false;

void usage(){
	printf("syntax : echo_server <port> [-b]\n");
	printf("sample : echo_server 1234 -b\n");
}

int send_msg(char * msg, int childfd){

	 if(option){
		pthread_mutex_lock(&mutx);
		/* 클라이언트 수 만큼 같은 메시지를 전달한다 */
		for(int i=0; i<client_num; i++){
			ssize_t sent = send(client[i], msg, strlen(msg),0);
			if (sent == 0) {
				perror("send failed");
				return -1;
			}
		}
		pthread_mutex_unlock(&mutx);
	 }else{
		ssize_t sent = send(childfd, msg, strlen(msg), 0);
		if (sent == 0) {
			perror("send failed");
			return -1;
		}
	 }
	 return 0;
}


void * client_connect(void *argument){
	int childfd = *((int*)argument);
	while (true) {
		const static int BUFSIZE = 1024;
		char buf[BUFSIZE];
		ssize_t received = recv(childfd, buf, BUFSIZE - 1, 0);
		if (received == 0 || received == -1) {
			perror("recv failed");
			break;
		}
		buf[received] = '\0';
		if(send_msg(buf,childfd)==-1){
			break;
		}
		printf("%s\n", buf);
	}

	pthread_mutex_lock(&mutx);
	for(int i=0; i<client_num; i++){
		if(childfd == client[i]){
			for( ; i<client_num-1; i++)
				client[i] = client[i+1];
			break;
		}
	}
	client_num--;
	pthread_mutex_unlock(&mutx);
}


int main(int argc, char* argv[]) {
	if(argc!=3 && argc!=2){
		usage();
	}
	option =true;
	if(pthread_mutex_init(&mutx,NULL)){
		printf("Mutex init error!\n");
		return -1;
	}
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket failed");
		return -1;
	}
	pthread_t pthread;

	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,  &optval , sizeof(int));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
	if (res == -1) {
		perror("bind failed");
		return -1;
	}

	res = listen(sockfd, 2);
	if (res == -1) {
		perror("listen failed");
		return -1;
	}

	while (true) {
		struct sockaddr_in addr;
		socklen_t clientlen = sizeof(sockaddr);
		int childfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr), &clientlen);
		if (childfd < 0) {
			perror("ERROR on accept");
			break;
		}
		pthread_mutex_lock(&mutx);
		client[client_num++] = childfd;
		pthread_mutex_unlock(&mutx);
		pthread_create(&pthread,NULL,client_connect,(void *)&childfd);
		printf("connected\n");
	}
	close(sockfd);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define PORT 3838
#define CONNECTION_QUEUE_LIMIT 10

void handler(int sockt, int pipefd);

int main(int argc, char ** argv) {
	int sockfd, newsockfd, port, clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n, pid;
	int pfd[2];
	pipe(pfd);
	//set socket file descriptors
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Socket Error");
		exit(1);
	}
	//init socket struct
	bzero((char *) &serv_addr, sizeof(serv_addr));
	//set some server info
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);
	//bind address
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Bind Error");
		exit(1);
	}
	//listen for clients
	listen(sockfd, CONNECTION_QUEUE_LIMIT);
	clilen = sizeof(cli_addr);
	//make a this struct to check if 
	struct pollfd * poll_fd = malloc(sizeof(struct pollfd));
	//enter loop
	while(1) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			perror("Error accepting connection");
			exit(1);
		}
		pid = fork();
		int status;
		switch(pid) {
			case 0:
				close(sockfd);
				handler(newsockfd, pfd[1]);
				exit(1);
			case -1:
				perror("Error Forking");
				exit(1);
			default:
				if (waitpid(-1, &status, 0) == -1) {
					perror("Wait failed");
					exit(1);
				}
				//check if child failed execution
				if (!WEXITSTATUS(status)) {
					char * buff = malloc(1000);
					poll_fd->fd = pfd[0];
					poll_fd->events = POLLIN;
					//check if pfd has something to read
					if (poll(poll_fd, 1, 0) == 1){
						read(pfd[0], buff, 1000);
						char * html = malloc(1000);
						sprintf(html, "HTTP/1.1 200 OK\nContent-Type: text/html\r\n\r\n<html><body>%s</body></html>\r\n", buff);
						send(newsockfd, html, strlen(html), 0);
					}
					//nothing to read, display 
					else {
						char * blank = "HTTP/1.1 200 OK\nContent-Type: text/html\r\n\r\n<html><body></body></html>\r\n";
						send(newsockfd, blank, strlen(blank), 0);
					}
				}
				close(newsockfd);
				break;
		}
	}
	return 0;
}

void handler(int sockt, int pipefd) {
	char * buffer = malloc(1000);
	recv(sockt, buffer, 1000, 0);
	char * cmd = strtok(buffer, "/");
	//ignore browser request for favicons
	if (strcmp(cmd, "favicon.ico") != 0){
		cmd = strtok(NULL, " ");
		dup2(pipefd, STDOUT_FILENO);
		execlp(cmd, cmd, NULL);	
	}
}
















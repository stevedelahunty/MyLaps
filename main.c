/*
 * Copyright (c) 2014, Steve Delahunty
 * All Rights Reserved
 *
 * This is a sample application that works with the MyLaps ToolKit
 * application as a the client for a "TCP EXPORTER". It's meant to
 * show others how a C program can be used to receive Bibs from the
 * MyLaps timing system.
 *
 * If you have any questions, you may contact the author, Steve Delahunty,
 * at the following:
 * 
 *    steve.delahunty@gmail.com
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOOLKIT_RECORDS   64


void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0) ;
}

int socks_listen(int port)
{
	int ret;
	int sockfd;   // listen on sock_fd
	struct sockaddr_in my_addr;	// my address information
	struct sigaction sa;
	int yes=1;

	/*
     * Create a socket and begin listening for incoming client connections.
     */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	ret=setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if (ret < 0) {
		perror("setsockopt");
		return(ret);
	}
	
	my_addr.sin_family = AF_INET;		 // host byte order
	my_addr.sin_port = htons(port);	 // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
	
	ret=bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr);
	if (ret < 0) {
		perror("bind");
		return(ret);
	}

	if ((ret=listen(sockfd, 5)) == -1) {
		perror("listen");
		return(ret);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	return(sockfd);
}


main()
{
	int port = 3097;
	char *name = "myclient";
	int listen_fd;
	int i, new_fd, count;
	char *remoteip;
	struct sockaddr_in their_addr; // connector's address information
	socklen_t sin_size;
	char *ptr;
	char line[4096], tmp[80], response[512];
	char *source, *function, *data[MAX_TOOLKIT_RECORDS];
	int num_records;
	int msgnum;
	int bib;
	int bibCount = 0;
	
	// Establish our listener port
	printf("Listening for toolkit on port %d\n", port);
	listen_fd = socks_listen(port);
	if (listen_fd < 0) {
		printf("Unable to create a listener socket. Goodbye!\n");
		exit(0);
	}
	

	// And wait for incoming connections
	while(1) {
		sin_size = sizeof(their_addr);
		printf("Waiting for an incoming connection....\n");
		
		new_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd < 0) {
			perror("accept");
			continue;
		}
		remoteip = (char *)inet_ntoa(their_addr.sin_addr);
		printf("server: got connection from %s\n",remoteip);
		
		/*
		 * Get the command
		 */
		while (1) {
			//printf("Reading something from MyLaps...\n");
			count = read(new_fd, line, sizeof(line));
			//printf("Count = %d\n", count);
			
			if (count <= 0) {
				break;
			}
			
			ptr = strchr(line,'\r');
			if (ptr) *ptr=0;
			printf("Read %d bytes. (%s)\n", count, line);
			memset(data, 0, sizeof(data));
			
			source = strtok(line, "@");
			function = strtok(NULL, "@");
			printf("RECEIVED: (%s) (%s) \n", source, function);
			if (!strcmp(function, "Store")) {
				printf("---> This is a STORE COMMAND\n");
				num_records=0;
				do {
					ptr = strtok(NULL, "@");
					if (ptr == NULL) break;
					printf("              (%s)\n", ptr);
					data[num_records] = ptr;
					num_records++;
				} while (num_records < MAX_TOOLKIT_RECORDS);
				printf("---> Number of records = %d\n", num_records);
				for (i=0; i<num_records-2; i++) {
					//printf("   (%s)\n", data[i]);
					strncpy(tmp, data[i], 7);
					tmp[7] = 0;
					if (!strncmp(tmp, "010", 3) || 1) {
						bib = atoi(tmp+3);
						printf("(%d)  BIB: %d\n", ++bibCount, bib);
					}
				}
				msgnum = atoi(data[num_records-2]);
				printf("---> Message number = %d\n", msgnum);
				memset(line, 0, sizeof(line));
				
				sprintf(line, "%s@AckStore@%d@$\r\n",
						name, msgnum);
				//printf("Responding with: (%s)\n", line);
				
				count = write(new_fd, line, strlen(line));
				//printf("Wrote %d bytes.\n", count);
				
			} else {
				data[0] = strtok(NULL, "@");
				num_records=1;
			}
			
			if (!strcmp(function, "Pong")) {
				//printf("--- PONG command\n");
				sprintf(line, "%s@AckPong@$\r\n", name);
				count = write(new_fd, line, strlen(line));
				//printf("We wrote %d bytes.\n", count);
			}
		}
		close(new_fd);
		continue;
	}


	
	exit(0);
}

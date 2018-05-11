#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT (12345)
#define LISTENNQ (5)
#define MAXLINE (100)
#define MAXTHREAD (5)

void* request_func(void *args);

int main(int argc, char **argv)
{
  int listenfd, connfd;

  struct sockaddr_in servaddr, cliaddr;
  socklen_t len = sizeof(struct sockaddr_in);

  char ip_str[INET_ADDRSTRLEN] = {0};

	int threads_count = 0;
	pthread_t threads[MAXTHREAD];


  /* initialize server socket */
  listenfd = socket(AF_INET, SOCK_STREAM, 0); /* SOCK_STREAM : TCP */
  if (listenfd < 0) {
          printf("Error: init socket\n");
          return 0;
  }

  /* initialize server address (IP:port) */
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY; /* IP address: 0.0.0.0 */
  servaddr.sin_port = htons(SERVER_PORT); /* port number */

  /* bind the socket to the server address */
  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
          printf("Error: bind\n");
          return 0;
  }

  if (listen(listenfd, LISTENNQ) < 0) {
          printf("Error: listen\n");
          return 0;
  }

  /* keep processing incoming requests */
  while (1) {
    /* accept an incoming connection from the remote side */
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
    if (connfd < 0) {
      printf("Error: accept\n");
      return 0;
    }

    /* print client (remote side) address (IP : port) */
    inet_ntop(AF_INET, &(cliaddr.sin_addr), ip_str, INET_ADDRSTRLEN);
    printf("Incoming connection from %s : %hu with fd: %d\n", ip_str, ntohs(cliaddr.sin_port), connfd);

		/* create dedicate thread to process the request */
		if (pthread_create(&threads[threads_count], NULL, request_func, (void *)connfd) != 0) {
			printf("Error when creating thread %d\n", threads_count);
			return 0;
		}

		if (++threads_count >= MAXTHREAD) {
			break;
		}

  }
	printf("Nax thread number reached, wait for all threads to finish and exit...\n");
	for (int i = 0; i < MAXTHREAD; ++i) {
		pthread_join(threads[i], NULL);
	}

  return 0;
}




void* request_func(void *args)
{
	/* get the connection fd */
	int connfd = (int)args;
        char buff[MAXLINE] = {0};

	printf("heavy computation\n");
	sleep(10);
       
  	/* prepare for the send buffer */ 
        snprintf(buff, sizeof(buff) - 1, "This is the content sent to connection %d\r\n", connfd);

	/* write the buffer to the connection */
	write(connfd, buff, strlen(buff));
	
	close(connfd);
}

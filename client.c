#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>


#define MAXLINE 100
#define SERVER_PORT 12345




char test1[] = "GET /test.jpg HTTP/1.1\r\n xixii hahdfafd asdf\r\n fasdfa\r\n";

int main(int argc, char *argv[])
{
        int sockfd, n, bytes_wrt, len;
      	char *filename;
        struct sockaddr_in servaddr;
        char rcv_buff[MAXLINE], wrt_buff[MAXLINE];
        memset(&wrt_buff, 0, sizeof(wrt_buff));
        memset(&rcv_buff, 0, sizeof(rcv_buff));

        if (argc != 2) {
                printf("Usage: %s [file name]\n", argv[0]);
                return 0;
        }

        filename = argv[1];

	printf("Fetch file '%s' from server ...\n", filename);

        /* init socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);	/* TCP */
        if (sockfd < 0) {
                printf("Error: init socket\n");
                return 0;
        }

        /* init server address (IP : port) */
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        servaddr.sin_port = htons(SERVER_PORT);

        /* connect to the server */
        if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                printf("Error: connect\n");
                return 0;
        }

        /* write the request */
        sprintf(wrt_buff, "%s\n", test1);
        bytes_wrt = 0;
        len = strlen(wrt_buff);
        while (bytes_wrt < len) {
                bytes_wrt += write(sockfd, wrt_buff + bytes_wrt, len - bytes_wrt);
        }

        /* read the response */
        while (1) {
                n = read(sockfd, rcv_buff, sizeof(rcv_buff) - 1);
                if (n <= 0) {
                        break;
                } else {
                        rcv_buff[n] = 0;        /* 0 terminate */
                        printf("%s", rcv_buff);
                }
        }
        printf("\n");

        /* close the connection */
        close(sockfd);
        return 0;
}

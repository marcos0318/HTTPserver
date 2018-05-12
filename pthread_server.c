#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/stat.h>

#define SERVER_PORT (12345)
#define LISTENNQ (5)
#define MAXLINE (100)

#define SERVER_STRING "Server: HTTPSERVER_JBAI"
enum FileType {
  HTML,
  CSS,
  JPG,
  PDF,
  PPTX
};

void* request_func(void* arg);
int read_line(int sock, char* buffer, int size);
void not_found(int connfd);
int exists(const char *fname);
void respondHTML(int connfd, const char* filepath);
void respondCSS(int connfd, const char* filepath);

// try to use the semaphore to do the synchronization things.
// but here we only need to read from the server.
// need the semaphore to limit the access to the resources

int main(int argc, char **argv) {
  int listenfd, connfd;

  struct sockaddr_in servaddr, cliaddr;
  socklen_t len = sizeof(struct sockaddr_in);

  char ip_str[INET_ADDRSTRLEN] = {0};

  pthread_t newthread;

  /* initialize server socket */
  listenfd = socket(AF_INET, SOCK_STREAM, 0); /* SOCK_STREAM : TCP */
  if (listenfd < 0) {
          printf("Error: init socket\n");
          return 0;
  }

  int threads_count = 0;

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

    
    int *connfdptr = malloc(sizeof(*connfdptr));
    *connfdptr = connfd;
		if (pthread_create(&newthread, NULL, request_func, connfdptr) != 0) {
			printf("Error when creating thread %d\n", threads_count);
			return 0;
		}

    threads_count++;
  }

	close(listenfd);
  return 0;
}

void* request_func(void* connfdptr) { 
  char rcv_buff[1024];
  char method[255];
  char url[255];

 
  size_t bytes_rcv;
 
	/* get the connection fd */
  int connfd = *((int*)connfdptr);
  char buff[MAXLINE] = {0};


  /* read the response */

  bytes_rcv = 0;
  while (1) {
    bytes_rcv += recv(connfd, rcv_buff + bytes_rcv, sizeof(rcv_buff) - bytes_rcv - 1, 0);
    if (bytes_rcv && rcv_buff[bytes_rcv - 1] == '\n')
    break;
  }

  
  sscanf(rcv_buff, "%s %s\n", method, url);
  printf("%s %s\n", method, url);

  /* prepare for the send buffer */ 
  snprintf(buff, sizeof(buff) - 1, "This is the content sent to connection %d\r\n", connfd);


  /*
	 write the buffer to the connection 
	write(connfd, method, strlen(method));
  write(connfd, url, strlen(url));
  */

  if (strcmp(url, "/") == 0) {
    respondHTML(connfd, "index.html");
  } 
  else {
    // try to parse what kind of file it is
    char extension_name[128];
    char * pch;
    char path_to_file[255];

    pch=strchr(url,'.');
    pch++;
    sprintf(extension_name, "%s", pch);
    sprintf(path_to_file, ".%s", url);
    if (strcmp(extension_name, "css") == 0) {
      printf("here\n");
      respondCSS(connfd, path_to_file);
    } 
    else if (strcmp(extension_name, "html") == 0) {
      respondHTML(connfd, path_to_file);
    } 
    else if (strcmp(extension_name, "pptx") == 0) {
      not_found(connfd);
    } 
    else if (strcmp(extension_name, "pdf") == 0) {
      not_found(connfd);
    } 
    else if (strcmp(extension_name, "jpg") == 0) {
      not_found(connfd);
    } 

    else {
      not_found(connfd);
    }
    // 
  }
  
	



  free(connfdptr);
	close(connfd);
}

void respondCSS(int connfd, const char* filepath) {
  char buf[1024];
  char* file_buffer = 0;
  printf("%s\n", filepath);
  if (exists(filepath)) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(connfd, buf, strlen(buf), 0);

    // data and time 
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
    send(connfd, buf, strlen(buf), 0);

    // server infomation
    sprintf(buf, SERVER_STRING);
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);

    // get last modified date
    struct stat attrib;
    stat(filepath, &attrib);
    strftime(buf, sizeof buf, "Last-Modified: %a, %d %b %Y %H:%M:%S %Z\r\n", gmtime(&(attrib.st_ctime)));
    send(connfd, buf, strlen(buf), 0);

    
    FILE *file;
    file = fopen(filepath, "r");

    fseek (file, 0, SEEK_END);
    int length = ftell (file);
    sprintf(buf, "Content-Length: %d\r\n", length);
    send(connfd, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/css\r\n");
    send(connfd, buf, strlen(buf), 0);



    sprintf(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);


    fseek (file, 0, SEEK_SET);
    file_buffer = malloc (length);
    fread (file_buffer, 1, length, file);
    if (file_buffer) {
      send(connfd, file_buffer, length, 0);
    } 


    fclose(file);
        

  }
  else {
    not_found(connfd);
  }
}

void respondHTML(int connfd, const char* filepath) {
  char buf[1024];
  char* file_buffer = 0;
  if (exists(filepath)) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(connfd, buf, strlen(buf), 0);

    // data and time 
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
    send(connfd, buf, strlen(buf), 0);

    // server infomation
    sprintf(buf, SERVER_STRING);
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);

    // get last modified date
    struct stat attrib;
    stat(filepath, &attrib);
    strftime(buf, sizeof buf, "Last-Modified: %a, %d %b %Y %H:%M:%S %Z\r\n", gmtime(&(attrib.st_ctime)));
    send(connfd, buf, strlen(buf), 0);

    
    FILE *file;
    file = fopen(filepath, "r");

    fseek (file, 0, SEEK_END);
    int length = ftell (file);
    sprintf(buf, "Content-Length: %d\r\n", length);
    send(connfd, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/html\r\n");
    send(connfd, buf, strlen(buf), 0);



    sprintf(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);


    fseek (file, 0, SEEK_SET);
    file_buffer = malloc (length);
    fread (file_buffer, 1, length, file);
    if (file_buffer) {
      send(connfd, file_buffer, length, 0);
    } 


    fclose(file);
        



  }
  else {
    not_found(connfd);
  }
}

void not_found(int connfd) {
 char buf[1024];

 sprintf(buf, "HTTP/1.1 404 NOT FOUND\r\n");
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, "your request because the resource specified\r\n");
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, "is unavailable or nonexistent.\r\n");
 send(connfd, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(connfd, buf, strlen(buf), 0);
}

int exists(const char *fname) {
    printf("%s\n", fname);
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}
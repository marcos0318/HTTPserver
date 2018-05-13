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
#include <assert.h>
#include "zlib.h"

#define SERVER_PORT (12345)
#define LISTENNQ (5)
#define MAXLINE (100)
#define CHUNK 16384

#define windowBits 15
#define GZIP_ENCODING 16

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
void respondPDF(int connfd, const char* filepath);
void respondPPTX(int connfd, const char* filepath);
void respondJPG(int connfd, const char* filepath);
void respondChunked(int connfd);
void respondZippedHTML(int connfd, const char* filepath);
void not_found(int connfd);
int def(FILE *source, FILE *dest, int level);

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
    respondZippedHTML(connfd, "index.html");
  } 
  else if (strcmp(url, "/chunked") == 0) {
    respondChunked(connfd);
  }
  else {
    // try to parse what kind of file it is
    char extension_name[128];
    char * pch = 0;
    char path_to_file[255];

    pch=strchr(url,'.');
    if (pch == 0) {
      not_found(connfd);
    }
    pch++;
    sprintf(extension_name, "%s", pch);
    sprintf(path_to_file, ".%s", url);
    if (strcmp(extension_name, "css") == 0) {
      respondCSS(connfd, path_to_file);
    } 
    else if (strcmp(extension_name, "html") == 0) {
      respondHTML(connfd, path_to_file);
    } 
    else if (strcmp(extension_name, "ppt") == 0 || strcmp(extension_name, "pptx") == 0) {
      respondPPTX(connfd, path_to_file);
    } 
    else if (strcmp(extension_name, "pdf") == 0) {
      respondPDF(connfd, path_to_file);
    } 
    else if (strcmp(extension_name, "jpg") == 0) {
      respondJPG(connfd, path_to_file);
    } 

    else {
      not_found(connfd);
    }
  }
  
  free(connfdptr);
	close(connfd);
}

void respondJPG(int connfd, const char* filepath) {
  char buf[10240];
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
               
    sprintf(buf, "Content-Type: image/jpeg\r\n");
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

void respondPPTX(int connfd, const char* filepath) {
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

    sprintf(buf, "Content-Type: application/vnd.openxmlformats-officedocument.presentationml.presentation\r\n");
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

void respondPDF(int connfd, const char* filepath) {
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

    sprintf(buf, "Content-Type: application/pdf\r\n");
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

void respondChunked(int connfd) {
  char buf[10240];
  sprintf(buf,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nDate: Mon, 28 Feb 2011 10:38:19 GMT\r\nTransfer-Encoding: chunked\r\nServer: Myserver\r\n\r\n%zx\r\n%s\r\n0\r\n\r\n",strlen("<html><body><p> This is the Chunked message </p> </body></html>"),"<html><body><p> This is the Chunked message </p> </body></html>");
  send(connfd, buf, strlen(buf), 0);
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

void respondZippedHTML(int connfd, const char* filepath) {
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

    char zippedPath[128];
    sprintf(zippedPath, "%s.zip", filepath);

    FILE *zippedfile;
    zippedfile = fopen(zippedPath, "w+");

    int ret = def(file, zippedfile, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
       not_found(connfd);



    fseek (zippedfile, 0, SEEK_END);
    int length = ftell (zippedfile);
    sprintf(buf, "Content-Length: %d\r\n", length);
    send(connfd, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/html\r\n");
    send(connfd, buf, strlen(buf), 0);

    sprintf(buf, "Content-Encoding: gzip\r\n");
    send(connfd, buf, strlen(buf), 0);

    sprintf(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);


    fseek (zippedfile, 0, SEEK_SET);
    file_buffer = malloc (length);
    fread (file_buffer, 1, length, zippedfile);
    if (file_buffer) {
      send(connfd, file_buffer, length, 0);
    } 


    fclose(file);
    fclose(zippedfile);
    remove(zippedPath);
  }
  else {
    not_found(connfd);
  }
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


int def(FILE *source, FILE *dest, int level) {
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2(&strm, level, Z_DEFLATED, windowBits | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}


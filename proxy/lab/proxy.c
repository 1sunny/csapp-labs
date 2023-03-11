#include "csapp.h"
#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
        "Firefox/10.0.3\r\n";

void doit(int fd);

void read_requesthdrs(rio_t *rp);

int parse_uri(char *uri, char *filename, char *cgiargs);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen); //line:netp:tiny:accept
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
                port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);                                             //line:netp:tiny:doit
    Close(connfd);                                            //line:netp:tiny:close
  }
}

int parseUrl(char* url, char* uri, char* host, char* port) {
  int n = strlen(url);
  char *p = strstr(url, "://");
  if(p == NULL){
    *port = *host = 0;
    return 0;
  }
  if(strncasecmp(url, "http://", 7) != 0 && strncasecmp(url, "https://", 8) != 0){
    fprintf(stderr, "Protocol Error");
    return -1;
  }
  p += 3;
  char* uri_start = strchr(p, '/');
  if(uri_start == NULL){
    *(url+n) = '/';
    uri_start = url+n;
  }
  strcpy(uri, uri_start);
  char *colon = strchr(p, ':');
  if(colon == NULL){
    strncpy(host, p, uri_start - p);
    port = "80";
  }else{
    strncpy(host, p, colon - p);
    strncpy(port, colon+1, uri_start - (colon+1));
  }
  return 0;
}

int parseVersion(char* v) {
  int n = strlen(v);
  *(v + n - 1) = '0';
  return 0;
}

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) {
  char buf[MAXLINE], method[MAXLINE], url[MAXLINE], uri[MAXLINE],
       version[MAXLINE], hostname[MAXLINE], port[MAXLINE];
  rio_t rio;
  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) == 0)
    return;
  sscanf(buf, "%s %s %s", method, url, version);
  if (strcasecmp(method, "GET") == 0) {
    if(parseUrl(url, uri, hostname, port) < 0){
      return;
    }
    if(*hostname == 0){}
    if(parseVersion(version) < 0){
      return;
    }
    int clientfd = Open_clientfd(hostname, port);
    if (clientfd < 0){
      fprintf(stderr, "Clientfd Error");
      exit(1);
    }
    rio_t client_rio;
    Rio_readinitb(&client_rio, clientfd);
    sprintf(buf, "%s %s %s\r\n", method, uri, version);
    Rio_writen(clientfd, buf, strlen(buf));
    // headers
    int host = 0, ua = 0, conn = 0, proxy_conn = 0;
    Rio_readlineb(&rio, buf, MAXLINE);
    while (strcmp(buf, "\r\n") != 0) {
      Rio_writen(clientfd, buf, strlen(buf));
      if(strstr(buf, "Host:") != NULL)
        host = 1;
      else if(strstr(buf, "User-Agent:") != NULL)
        ua = 1;
      else if(strstr(buf, "Connection:") != NULL)
        conn = 1;
      else if(strstr(buf, "Proxy-Connection:") != NULL)
        proxy_conn = 1;
      Rio_readlineb(&rio, buf, MAXLINE);
    }
    if(host == 0){
      sprintf(buf, "Host: %s\r\n", hostname);
      Rio_writen(clientfd, buf, strlen(buf));
    }
    if(ua == 0){
      Rio_writen(clientfd, user_agent_hdr, strlen(user_agent_hdr));
    }
    if(conn == 0){
      sprintf(buf, "Connection: close\r\n");
      Rio_writen(clientfd, buf, strlen(buf));
    }
    if(proxy_conn == 0){
      sprintf(buf, "Proxy-Connection: close\r\n");
      Rio_writen(clientfd, buf, strlen(buf));
    }
    Rio_writen(clientfd, "\r\n", 2);
    // send to client
    ssize_t n;
    while((n = Rio_readlineb(&client_rio, buf, MAXLINE)) && n > 0){
      Rio_writen(fd, buf, n);
    }
    Close(clientfd);
  }else{
    fprintf(stderr, "Method Not Supported");
    exit(1);
  }
}
/* $end doit */
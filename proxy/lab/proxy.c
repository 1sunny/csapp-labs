#include "csapp.h"
#include "sbuf.h"
#include <stdio.h>

#define NTHREADS 4
#define SBUFSIZE 16

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
        "Firefox/10.0.3\r\n";

void doit(int fd);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

sbuf_t sbuf;

void *thread(void *vargp) {
  Pthread_detach(pthread_self());
  while (1) {
    int connfd = sbuf_remove(&sbuf);
    doit(connfd);
    Close(connfd);
  }
}

int main(int argc, char **argv) {
  int listenfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;
  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  sbuf_init(&sbuf, SBUFSIZE);
  for (int i = 0; i < NTHREADS; ++i) {
    Pthread_create(&tid, NULL, thread, NULL);
  }

  while (1) {
    clientlen = sizeof(clientaddr);
    int connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
    //Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    //printf("Accepted connection from (%s, %s)\n", hostname, port);
    sbuf_insert(&sbuf, connfd);
  }
}

int parseUrl(char *url, char *uri, char *host, char *port) {
  int n = strlen(url);
  char *p = strstr(url, "://");
  // client is proxy too, host and port from Header.Host ?
  if (p == NULL) {
    *port = *host = 0;
    return 0;
  }
  // support http only
  if (strncasecmp(url, "http://", 7) != 0) {
    fprintf(stderr, "Protocol Error");
    return -1;
  }
  p += 3;
  char *uri_start = strchr(p, '/');
  if (uri_start == NULL) {
    *(url + n) = '/';
    uri_start = url + n;
  }
  strcpy(uri, uri_start);
  char *colon = strchr(p, ':');
  if (colon == NULL) {
    strncpy(host, p, uri_start - p);
    port = "80";
  } else {
    strncpy(host, p, colon - p);
    strncpy(port, colon + 1, uri_start - (colon + 1));
  }
  return 0;
}

int parseVersion(char *v) {
  if (strcasecmp(v, "http/1.1") == 0) {
    *(v + strlen(v) - 1) = '0';
    return 0;
  }
  return -1;
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
  if (Rio_readlineb(&rio, buf, MAXLINE) == 0) {
    return;
  }
  sscanf(buf, "%s %s %s", method, url, version);
  if (strcasecmp(method, "GET") == 0) {
    if (parseUrl(url, uri, hostname, port) < 0) {
      clienterror(fd, "", "500", "", "parseUrl Error");
      return;
    }
    if (*hostname == 0) {}
    if (parseVersion(version) < 0) {
      clienterror(fd, "", "500", "", "parseVersion Error");
      return;
    }
    int clientfd = Open_clientfd(hostname, port);
    if (clientfd < 0) {
      clienterror(fd, "", "500", "", "Clientfd Error");
      return;
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
      if (strstr(buf, "Host:") != NULL)
        host = 1;
      else if (strstr(buf, "User-Agent:") != NULL)
        ua = 1;
      else if (strstr(buf, "Connection:") != NULL)
        conn = 1;
      else if (strstr(buf, "Proxy-Connection:") != NULL)
        proxy_conn = 1;
      Rio_readlineb(&rio, buf, MAXLINE);
    }
    if (host == 0) {
      sprintf(buf, "Host: %s\r\n", hostname);
      Rio_writen(clientfd, buf, strlen(buf));
    }
    if (ua == 0) {
      Rio_writen(clientfd, user_agent_hdr, strlen(user_agent_hdr));
    }
    if (conn == 0) {
      sprintf(buf, "Connection: close\r\n");
      Rio_writen(clientfd, buf, strlen(buf));
    }
    if (proxy_conn == 0) {
      sprintf(buf, "Proxy-Connection: close\r\n");
      Rio_writen(clientfd, buf, strlen(buf));
    }
    Rio_writen(clientfd, "\r\n", 2);
    // send to client
    ssize_t n;
    while ((n = Rio_readlineb(&client_rio, buf, MAXLINE)) && n > 0) {
      Rio_writen(fd, buf, n);
    }
    Close(clientfd);
  } else {
    clienterror(fd, "", "500", "", "Method Not Supported");
  }
}
/* $end doit */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg) {
  char buf[MAXLINE];

  /* Print the HTTP response headers */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* Print the HTTP response body */
  sprintf(buf, "<html><title>Tiny Error</title>");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
  Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */

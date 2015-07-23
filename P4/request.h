#ifndef __REQUEST_H__
struct netd_t {
    int requestID;
    int size;
    int fd;
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio; 
};

void requestHandle(struct netd_t* nd);
void requestFN(struct netd_t* nd);
#endif

#include "cs537.h"
#include "request.h"
#include <pthread.h>
#include <sys/stat.h>

static int const BAD_VALUE = 2000000000;
//#define _SORT_DEBUG; 

pthread_cond_t empty, fill;
pthread_mutex_t mutex;
pthread_t master;

int port;
int buffer_size;
struct netd_t* cds; //connections buffer
int to_fill = 0;
int use = 0;
int count = 0;
char* policy;

void getargs(int *port, int *threads, int *buffer_size, int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <port> <threads> <buffers> <policy>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffer_size = atoi(argv[3]);
    policy = argv[4];
    
    //buffer size must be a positive integer
    if (*buffer_size < 1) {
        fprintf(stderr, "Usage: buffer size must be positive!\n");
        exit (1);
    }
    
    //must enter valid policy type
    if (strcmp(policy, _FIFO_DEFINE) != 0 && strcmp(policy, _SFNF_DEFINE) != 0
        && strcmp(policy, _SFF_DEFINE) != 0) {
        fprintf(stderr, "Usage: invalid policy entered!\n");
        exit(1);
    }
}

int compare_func(const void * a, const void * b) {
    const struct netd_t* nd_a = (struct netd_t*)a;
    const struct netd_t* nd_b = (struct netd_t*)b;
    
    if (nd_a->size > nd_b->size) return 1;
    else if (nd_a->size == nd_b->size) return 0;
    else return -1;
}

void put(int value) {
    
    //FIFO policy handler
    if (strcmp(policy, _FIFO_DEFINE) == 0) {
        cds[to_fill].fd = value;
        requestFN(&cds[to_fill]);
        to_fill = (to_fill + 1) % buffer_size;
        count++;
    
    //SFF policy handler
    } else if (strcmp(policy, _SFF_DEFINE) == 0) {
        cds[count].fd = value;
        requestFN(&cds[count]);
        //get filesize
        struct stat filestat;
        Stat(cds[count].filename, &filestat);
        cds[count].size = filestat.st_size;
        //sort out
        qsort(cds, buffer_size, sizeof(struct netd_t), compare_func);
        count++;

    //SFNF policy handler
    } else if (strcmp(policy, _SFNF_DEFINE) == 0) {
	cds[count].fd = value;
        requestFN(&cds[count]);
        //get filename size
	int size = (int)(strlen(cds[count].filename));
	if (size == 0) size = BAD_VALUE;
        cds[count].size = size;
        //sort out
        qsort(cds, buffer_size, sizeof(struct netd_t), compare_func);
        count++;
    }
}

struct netd_t get() {
    struct netd_t tmp;
    
    //FIFO policy handler
    if (strcmp(policy, _FIFO_DEFINE) == 0) {
        tmp = cds[use];
        use = (use + 1) % buffer_size;
        count--;
    //SFF & SFNF policy handler
    } else {
        tmp = cds[0];
        cds[0].size = BAD_VALUE;
        qsort(cds, buffer_size, sizeof(struct netd_t), compare_func);
        count--;
    }

    return tmp;
}

void* consumer(void *arg) {
    while (1) {
        Pthread_mutex_lock(&mutex);
        
        while (count == 0)
            Pthread_cond_wait(&fill, &mutex);
        
        struct netd_t nd = get();
        
        Pthread_cond_signal(&empty);
        Pthread_mutex_unlock(&mutex);
	
        requestHandle(&nd);
        Close(nd.fd);
    }
    return NULL;
}

void* producer(void *arg) {
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    
    listenfd = Open_listenfd(port);
	
    while (1) {
        //accept connections
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);        

        //place connection in buffer
        Pthread_mutex_lock(&mutex);
        while (count == buffer_size)
            Pthread_cond_wait(&empty, &mutex);
        put(connfd);
        Pthread_cond_signal(&fill);
        Pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    #ifdef _SORT_DEBUG
    //test by running requests with client.c
    printf("Running Sort Tests:\n");
    
    int threads;
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    
    getargs(&port, &threads, &buffer_size, argc, argv);
    listenfd = Open_listenfd(port);
    //create buffer
    cds = malloc(buffer_size * sizeof(struct netd_t));
    int itr = 0;
    for (itr = 0; itr < buffer_size; itr++) {
        cds[itr].size = BAD_VALUE;
    }
    
    while (1) {
        //accept connections
	if (count == buffer_size) {
	    struct netd_t tmp = get();
	    printf("Getting: [Filename: %s, Size: %i]", tmp.filename, tmp.size);
	}
	printf("\n");
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);        

        //place connection in buffer
        put(connfd);
	printf("Buffer - Post Put:\n");
	for (itr = 0; itr < buffer_size; itr++) {
	  printf("[Filename: %s, Size: %i]", cds[itr].filename, cds[itr].size);
	}
	printf("\n");
    }
    
    return 0;
    
    #else 
    int threads;
    pthread_t* workers; //worker table
    
    getargs(&port, &threads, &buffer_size, argc, argv);
    
    printf("Starting server...\n");
    
    //init locks and cvs
    Pthread_mutex_init(&mutex, NULL);
    Pthread_cond_init(&empty, NULL);
    Pthread_cond_init(&fill, NULL);
    
    //create buffer
    cds = malloc(buffer_size * sizeof(struct netd_t));
    int itr = 0;
    for (itr = 0; itr < buffer_size; itr++) {
        cds[itr].requestID = BAD_VALUE;
        cds[itr].size = BAD_VALUE;
    }
    
    //create master thread
    Pthread_create(&master, NULL, &producer, NULL); 
    
    //dynamically create worker threads
    workers = malloc(threads * sizeof(pthread_t));
    
    for (itr = 0; itr < threads; itr++) {
        Pthread_create(&workers[itr], NULL, &consumer, NULL);
    }
    
    (void) pthread_join(master, NULL);
    return 0;
    #endif
}


    


 

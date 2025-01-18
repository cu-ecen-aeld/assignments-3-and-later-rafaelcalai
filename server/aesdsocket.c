#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <pthread.h>
#include <time.h>
#include "queue.h"

#define MAX 1024
#define PORT 9000
#define SA struct sockaddr
#define LOG_FILE "/var/tmp/aesdsocketdata"

char *dynamic_buffer = NULL;
size_t dynamic_size = 0;
int sockfd;
struct sockaddr_in servaddr, cli;


pthread_mutex_t syslog_lock; 
pthread_mutex_t file_lock; 

typedef struct slist_data_s slist_data_t;
struct slist_data_s {
    pthread_t thread;
    bool is_completed;
    int connfd;
    char client_ip[INET_ADDRSTRLEN];
    SLIST_ENTRY(slist_data_s) entries;
};
SLIST_HEAD(slisthead, slist_data_s) head;
slist_data_t *tdatap=NULL;

void open_syslog(void);
void write_syslog(int priority, const char *msg);
void close_syslog(void);
void write_file(const char *write_string);
void* write_timestamp(void* arg);
void sig_handler(int signo);
void* func(void* arg);
void start_daemon(void);
void deallocate_likedlist(void);


// Driver function
int main(int argc, char *argv[])
{
    open_syslog();
    bool is_deamon = false;
    if (argc == 2)
    {
        if (strcmp(argv[1], "-d") == 0)
        {
            write_syslog(LOG_INFO, "received arg: -d and will run as deamon");
            is_deamon = true;
        }
    }

    if (pthread_mutex_init(&syslog_lock, NULL) != 0) { 
        write_syslog(LOG_ERR, "syslog_lock mutex init has failed\n"); 
        return 1; 
    } 
    if (pthread_mutex_init(&file_lock, NULL) != 0) { 
        write_syslog(LOG_ERR, "file_lock mutex init has failed\n"); 
        return 1; 
    }

    
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        write_syslog(LOG_ERR,"can't catch SIGINT\n");
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR)
    {
        write_syslog(LOG_ERR,"can't catch SIGTERM\n");
    }

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        write_syslog(LOG_ERR, "socket creation failed...\n");
        exit(-1);
    }

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        write_syslog(LOG_ERR, "socket bind failed...\n");
        exit(-1);
    }

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        write_syslog(LOG_ERR, "Listen failed...\n");
        exit(-1);
    }

    if (is_deamon)
    {
        start_daemon();
    }

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, write_timestamp, NULL);

    slist_data_t *datap=NULL;
    SLIST_INIT(&head);

    while (1)
    {
        socklen_t client_addrlen = sizeof(cli);
        datap = calloc(1, sizeof(slist_data_t));
        // Accept the data packet from client and verification
        datap->connfd = accept(sockfd, (SA *)&cli, &client_addrlen);
        if (datap->connfd < 0)
        {
            write_syslog(LOG_ERR, "server accept failed...\n");
            free(datap);
            exit(-1);
        }
        else
        {
            datap->is_completed = false;
            inet_ntop(AF_INET, &cli.sin_addr, datap->client_ip, INET_ADDRSTRLEN);
            char s[100] = {0};
            sprintf(s,"Accepted connection from %s\n", datap->client_ip);                   
            write_syslog(LOG_INFO, s);
            pthread_create(&datap->thread, NULL, func, datap);
        }
        slist_data_t *elem=calloc(1, sizeof(slist_data_t));
        SLIST_FOREACH_SAFE(elem, &head, entries, tdatap)
        {
            if (elem->is_completed == true) 
            {
                close(elem->connfd);
                pthread_join(elem->thread, NULL);
                SLIST_REMOVE(&head, elem, slist_data_s , entries);
            }       
        }
        free(elem);
    }
}

void deallocate_likedlist(void){
    slist_data_t *elem=calloc(1, sizeof(slist_data_t));
    SLIST_FOREACH_SAFE(elem, &head, entries, tdatap)
    {
        close(elem->connfd);
        pthread_join(elem->thread, NULL);
        SLIST_REMOVE(&head, elem, slist_data_s , entries);
    }
    free(elem);
}
        

void sig_handler(int signo)
{
    write_syslog(LOG_INFO, "Caught signal, exiting\n");
    deallocate_likedlist();
    remove(LOG_FILE);
    free(dynamic_buffer);
    close(sockfd);
    remove(LOG_FILE);
    pthread_mutex_destroy(&syslog_lock);
    pthread_mutex_destroy(&file_lock);
    exit(0);
}

void* func(void* arg)
{
    slist_data_t* datap = (slist_data_t*) arg;
    ssize_t bytes_received;
    
    // Ensure buff is properly initialized to zero
    char* buff = (char* )calloc(MAX, sizeof(char));
    if (buff == NULL) {
        datap->is_completed = true;
        perror("calloc failed for buff");
        return NULL;
    }

    while ((bytes_received = recv(datap->connfd, buff, MAX, 0)) > 0)
    {
        // Ensure dynamic_buffer is initialized or expanded properly
        dynamic_buffer = realloc(dynamic_buffer, dynamic_size + bytes_received + 1);
        if (!dynamic_buffer)
        {
            perror("Realloc failed");
            free(dynamic_buffer);  // Ensure memory is freed if realloc fails
            exit(-1);
        }

        // Copy received data to dynamic buffer
        memcpy(dynamic_buffer + dynamic_size, buff, bytes_received);
        dynamic_size += bytes_received;
        dynamic_buffer[dynamic_size] = '\0';  // Null-terminate

        // Process and check for newline
        for (int i = 0; i < bytes_received; i++)
        {
            if (buff[i] == '\n')
            {
                write_file(buff);  // Write data to file

                int filefd;
                char *buffer;
                size_t buffer_size;
                struct stat file_stat;
                pthread_mutex_lock(&file_lock);
                filefd = open(LOG_FILE, O_RDONLY);
                if (filefd == -1) {
                    perror("Error opening file");
                    pthread_mutex_unlock(&file_lock);
                    return;
                }

                // Get the file size
                if (fstat(filefd, &file_stat) == -1) {
                    perror("Error getting file size");
                    close(filefd);
                    pthread_mutex_unlock(&file_lock);
                    return;
                }
                buffer_size = file_stat.st_size;

                // Allocate memory for the buffer
                buffer = (char *)malloc(buffer_size);
                if (buffer == NULL) {
                    perror("Error allocating memory");
                    close(filefd);
                    pthread_mutex_unlock(&file_lock);
                    return;
                }

                // Read the file content into the buffer
                ssize_t bytes_read = read(filefd, buffer, buffer_size);
                if (bytes_read == -1) {
                    perror("Error reading file");
                    free(buffer);
                    close(filefd);
                    pthread_mutex_unlock(&file_lock);
                    return;
                }

                // Send the file content to the client
                ssize_t bytes_written = write(datap->connfd, buffer, bytes_read);
                if (bytes_written == -1) {
                    perror("Error writing to client");
                } else if (bytes_written < bytes_read) {
                    fprintf(stderr, "Warning: Partial write occurred\n");
                }

                // Cleanup
                free(buffer);
                close(filefd);
                pthread_mutex_unlock(&file_lock);
                break;
            }
        }
    }

    if (bytes_received < 0)
    {
        perror("Recv failed");
    }
    else
    {
        write_syslog(LOG_DEBUG, "Client disconnected");
        char s[100] = {0};
        sprintf(s, "Closed connection from %s\n", datap->client_ip);
        write_syslog(LOG_INFO, s);
    }

    datap->is_completed = true;
    free(buff);  // Free allocated memory for buff
    return NULL;
}

void write_file(const char *write_string)
{
    FILE *fptr;
    pthread_mutex_lock(&file_lock);

    // Ensure the write string is properly null-terminated and valid
    if (write_string == NULL) {
        write_syslog(LOG_ERR, "Attempt to write null string to file.\n");
        pthread_mutex_unlock(&file_lock);
        return;
    }

    fptr = fopen(LOG_FILE, "a");
    if (fptr == NULL)
    {
        write_syslog(LOG_ERR, "Not able to open the file.\n");
        pthread_mutex_unlock(&file_lock);
        return;
    }
    
    // Ensure write_string is initialized properly
    if (fputs(write_string, fptr) == EOF) {
        write_syslog(LOG_ERR, "Error writing to file.\n");
    }
    // Force the file to be written to disk
    if (fsync(fptr) == -1) {
        perror("Error forcing file to disk");
    }
    fclose(fptr);
    pthread_mutex_unlock(&file_lock);
    
}



void start_daemon(void)
{
    pid_t pid = fork();
    if (pid == -1)
        exit(-1);
    else if (pid != 0)
        exit(EXIT_SUCCESS);
    /* create new session and process group */
    if (setsid() == -1)
        exit(-1);
    /* set the working directory to the root directory */
    if (chdir("/") == -1)
        exit(-1);

    /* redirect fd's 0,1,2 to /dev/null */
    open("/dev/null", O_RDWR); /* stdin */
    dup(0);                    /* stdout */
    dup(0);
}

void open_syslog()
{
    openlog(NULL, 0, LOG_USER);
    write_syslog(LOG_DEBUG, "Logging initialized.");
}

void close_syslog()
{
    closelog();
}



void write_syslog(int priority, const char *msg){
    pthread_mutex_lock(&syslog_lock); 
    syslog(priority, "%s", msg);
    pthread_mutex_unlock(&syslog_lock); 
}

void* write_timestamp(void* arg) 
{ 
    char outstr[200] = {0};
    time_t t;
    struct tm *tmp;

    for(int i = 0;i<10;i++){
        t = time(NULL);
        tmp = localtime(&t);
        if (tmp == NULL) {
            write_syslog(LOG_ERR,"localtime");
            exit(EXIT_FAILURE);
        }

        if (strftime(outstr, sizeof(outstr), "timestamp: %a, %d %b %Y %T\n", tmp) == 0) {
            write_syslog(LOG_ERR, "strftime returned 0");
            exit(EXIT_FAILURE);
        }
        write_file(outstr);
        sleep(10);
    }
    return NULL; 
} 
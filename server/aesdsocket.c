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

#define MAX 1024
#define PORT 9000
#define SA struct sockaddr
#define LOG_FILE "/var/tmp/aesdsocketdata"

char *dynamic_buffer = NULL;
size_t dynamic_size = 0;
int sockfd, connfd;
struct sockaddr_in servaddr, cli;

void open_syslog(void);
void close_syslog(void);
void write_file(const char *write_string);

void sig_handler(int signo)
{
    syslog(LOG_INFO, "Caught signal, exiting\n");
    remove(LOG_FILE);
    close(sockfd);
    exit(0);
}

void func(int connfd)
{
    char buff[MAX];
    ssize_t bytes_received;

    while ((bytes_received = recv(connfd, buff, MAX, 0)) > 0)
    {
        // Allocate or expand dynamic buffer
        char *temp = realloc(dynamic_buffer, dynamic_size + bytes_received + 1);
        if (!temp)
        {
            perror("Realloc failed");
            free(dynamic_buffer);
            exit(-1);
        }
        dynamic_buffer = temp;

        // Append received data to dynamic buffer
        memcpy(dynamic_buffer + dynamic_size, buff, bytes_received);
        dynamic_size += bytes_received;
        dynamic_buffer[dynamic_size] = '\0'; // Null-terminate the string

        printf("Received: %s\n", buff);
        write_file(buff);
        for (int i = 0; i < bytes_received; i++)
        {
            if (buff[i] == '\n')
            {
                write(connfd, dynamic_buffer, dynamic_size);
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
        printf("Client disconnected\n");
    }
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
// Driver function
int main(int argc, char *argv[])
{
    open_syslog();
    bool is_deamon = false;
    if (argc == 2)
    {
        if (strcmp(argv[1], "-d") == 0)
        {
            syslog(LOG_INFO, "received arg: %s and will run as deamon", argv[1]);
            is_deamon = true;
        }
    }
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        printf("\ncan't catch SIGINT\n");
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR)
    {
        printf("\ncan't catch SIGTERM\n");
    }
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        syslog(LOG_ERR, "socket creation failed...\n");
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
        syslog(LOG_ERR, "socket bind failed...\n");
        exit(-1);
    }

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        syslog(LOG_ERR, "Listen failed...\n");
        exit(-1);
    }

    if (is_deamon)
    {
        start_daemon();
    }

    struct hostent *hostp;
    while (1)
    {
        socklen_t client_addrlen = sizeof(cli);
        // Accept the data packet from client and verification
        connfd = accept(sockfd, (SA *)&cli, &client_addrlen);
        if (connfd < 0)
        {
            syslog(LOG_ERR, "server accept failed...\n");
            exit(-1);
        }
        else
        {
            hostp = gethostbyaddr((const char *)&cli.sin_addr.s_addr,
                                  sizeof(cli.sin_addr.s_addr), AF_INET);
            syslog(LOG_INFO, "Accepted connection from %s\n", hostp->h_name);
        }

        func(connfd);
        syslog(LOG_INFO, "Closed connection from %s\n", hostp->h_name);
    }
    remove(LOG_FILE);
    close(sockfd);
}

void open_syslog()
{
    openlog(NULL, 0, LOG_USER);
    syslog(LOG_DEBUG, "Logging initialized.");
}

void close_syslog()
{
    closelog();
}

void write_file(const char *write_string)
{
    FILE *fptr;

    fptr = fopen(LOG_FILE, "a");
    if (fptr == NULL)
    {
        syslog(LOG_ERR, "Not able to open the file.\n");
        return;
    }
    fprintf(fptr, "%s", write_string);
    fclose(fptr);
}
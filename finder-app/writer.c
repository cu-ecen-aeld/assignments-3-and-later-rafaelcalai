#include <stdio.h>
#include <string.h>
#include <syslog.h>

void open_syslog(void);
void close_syslog(void);
void write_file(const char* file_name, const char* write_string);

int main(const int arg,const char * argv[]){
    open_syslog();
    if (arg <= 2)
    {
        syslog(LOG_ERR,"Please provide two args: <writer file PATH> <writer string>! \n");
        close_syslog();
        return 1;
    }

    const char* file_path = argv[1];
    const char* file_string = argv[2];
    
    write_file(file_path, file_string);

    close_syslog();
    return 0;
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


void write_file(const char* file_name, const char* write_string)
{
    FILE *fptr;

    fptr = fopen(file_name, "w");
    if (fptr == NULL){
        syslog(LOG_ERR,"Not able to open the file.\n");
        return;
    }
    syslog(LOG_DEBUG,"Writer file: %s and Writer string: %s\n", file_name, write_string);
    fprintf(fptr, "%s" ,write_string);
    fclose(fptr);
}
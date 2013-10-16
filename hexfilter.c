#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <sys/wait.h>

#define max(a, b) (((a) > (b)) ? (a) : (b))

/*
 * TODO
 * 
 * 1. write a filter
 * 2. catch child's exit status
 */

static int bump(int src_fd, int dst_fd, ssize_t (*filter)(char*, ssize_t))
{
    char r_buffer[1024];
    char* cp_buffer;
    ssize_t r_bytes;
    ssize_t cp_bytes;

    r_bytes = read(src_fd, r_buffer, 1024);

    if(filter == NULL){
        cp_buffer = r_buffer;
        cp_bytes = r_bytes;
    }
    else{
        cp_bytes = filter(r_buffer, r_bytes);
        cp_buffer = r_buffer;
    }

    if(cp_bytes > 0){
        write(dst_fd, cp_buffer, cp_bytes);
        return cp_bytes;
    }
    else{
        return 0;
    }
}

int main(int argc, char *argv[])
{
    argc--;
    argv++;

    int stdin_p[2];
    if(pipe(stdin_p)){
        perror("Pipe failed.");
    }

    int stdout_p[2];
    if(pipe(stdout_p)){
        perror("Pipe failed.");
    }

    int stderr_p[2];
    if(pipe(stderr_p)){
        perror("Pipe failed.");
    }

    pid_t c_pid = fork();

    if (c_pid >= 0){
        if (c_pid == 0){
            /* child */
            dup2(stdin_p[0], STDIN_FILENO);
            close(stdin_p[1]);

            dup2(stdout_p[1], STDOUT_FILENO);
            close(stdout_p[0]);

            dup2(stderr_p[1], STDERR_FILENO);
            close(stderr_p[0]);

            execvp(argv[0], argv);

            perror("Execv failed.");
        }
        else {
            /* parent */
            int child_stdin = stdin_p[1];
            int child_stdout = stdout_p[0];
            int child_stderr = stderr_p[0];
            close(stdin_p[0]);
            close(stdout_p[1]);
            close(stderr_p[1]);

            fd_set rfds;
            int nfds;

            nfds = max(child_stdout, child_stderr);
            nfds = max(STDIN_FILENO, nfds);

            while(1){
                FD_ZERO(&rfds);
                FD_SET(child_stdout, &rfds);
                FD_SET(child_stderr, &rfds);
                FD_SET(STDIN_FILENO, &rfds);

                select(nfds+1, &rfds, NULL, NULL, NULL);

                if(FD_ISSET(STDIN_FILENO, &rfds)){
                    bump(STDIN_FILENO, child_stdin, NULL);
                }

                if(FD_ISSET(child_stdout, &rfds)){
                    bump(child_stdout, STDOUT_FILENO, NULL);
                }

                if(FD_ISSET(child_stderr, &rfds)){
                    bump(child_stderr, STDERR_FILENO, NULL);
                }
            }
        }
    }
    else {
        perror("Fork failed.");
        return 1;
    }

    return 0;
}

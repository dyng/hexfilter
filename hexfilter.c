#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

static int bump(int src_fd, int dst_fd, ssize_t (*filter)(const char*, size_t, char*, size_t))
{
    char r_buffer[1024];
    ssize_t r_len;
    char* w_buffer;
    ssize_t w_len;

    r_len = read(src_fd, r_buffer, 1024);

    if(filter == NULL){
        w_buffer = r_buffer;
        w_len = r_len;
    }
    else{
        /* first call: get the length of buffer for later processing. */
        w_len = filter(r_buffer, r_len, NULL, 0);

        w_buffer = (char*) malloc( sizeof(char) * w_len );
        if(w_buffer == NULL){
            return -1;
        }

        /* second call: actual processing */
        w_len = filter(r_buffer, r_len, w_buffer, w_len);

        if(w_len < 0){
            return -1;
        }
    }

    int ret;
    if(w_len > 0){
        ret = write(dst_fd, w_buffer, w_len);
    }
    else{
        ret = 0;
    }

    if (filter != NULL && w_buffer != NULL){
        free(w_buffer);
    }

    return ret;
}

static ssize_t htoa(const char* in, size_t in_len, char* out, size_t out_len)
{
    char str[5];

    if(out == NULL){
        return in_len * 4;
    }

    int in_idx, out_idx;
    for(in_idx = 0, out_idx = 0; in_idx < in_len; in_idx++){
        uint8_t c = in[in_idx];

        /* printable character */
        if((c >= 0x9 && c <= 0x0d ) || (c >= 0x20 && c <= 0x7e) || (c == 0x1b)){
            if (out_idx + 1 > out_len){
                return -1;
            }
            out[out_idx] = c;
            out_idx += 1;
        }
        /* non-printable character */
        else{
            if (out_idx + 4 > out_len){
                return -1;
            }
            sprintf(str, "\\x%02x", c);
            memcpy(out + out_idx, str, 4);
            out_idx += 4;
        }
    }

    return out_idx;
}

static ssize_t atoh(const char* in, size_t in_len, char* out, size_t out_len)
{
    char str[3] = "00";

    if(out == NULL){
        return in_len;
    }

    int in_idx, out_idx;
    for(in_idx = 0, out_idx = 0; in_idx < in_len; in_idx++){
        uint8_t c = in[in_idx];

        if(c == '\\' && in_idx+3 < in_len && in[in_idx+1] == 'x'){
            memcpy(str, in+in_idx+2, 2);
            uint8_t h = (uint8_t) strtol(str, NULL, 16);
            out[out_idx] = h;
            in_idx += 3;
            out_idx += 1;
        }
        else{
            out[out_idx] = c;
            out_idx += 1;
        }
    }

    return out_idx;
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
                    if(bump(STDIN_FILENO, child_stdin, atoh) < 0){
                        exit(-1);
                    };
                }

                if(FD_ISSET(child_stdout, &rfds)){
                    if(bump(child_stdout, STDOUT_FILENO, htoa) < 0){
                        exit(-1);
                    };
                }

                if(FD_ISSET(child_stderr, &rfds)){
                    if(bump(child_stderr, STDERR_FILENO, htoa) < 0){
                        exit(-1);
                    };
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

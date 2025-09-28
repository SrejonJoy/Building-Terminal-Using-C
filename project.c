#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>

pid_t pid = -1;
void handle_sigint(int sig) {
    printf("\n Type 'exit' to quit the shell.\n");
    fflush(stdout);
}

typedef enum {
    OP_NONE,
    OP_SEMICOLON,
    OP_AND
} OperatorType;

typedef struct {
    char *cmd;    
    OperatorType op; 
} CommandSegment;

char *remove_null_space(char *str) {
    while(isspace(*str)) str++;
    if(*str == 0)
        return str;
    char *end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) end--;
    *(end+1) = '\0';
    return str;
}

int parse_command_line(char *input, CommandSegment segments[], int max_segments) {
    int count = 0;
    char *start = input;
    char *p = input;
    while (*p && count < max_segments) {
        if (*p == ';') {
            *p = '\0';
            segments[count].cmd = remove_null_space(start);
            segments[count].op = OP_SEMICOLON;
            count++;
            p++;
            start = p;
        } else if (*p == '&' && *(p + 1) == '&') {
            *p = '\0';
            segments[count].cmd = remove_null_space(start);
            segments[count].op = OP_AND;
            count++;
            p += 2; 
            start = p;
        } else {
            p++;
        }
    }
    if (count < max_segments && *start) {
        segments[count].cmd = remove_null_space(start);
        segments[count].op = OP_NONE; 
        count++;
    }
    return count;
}

int command_processing(char *command) {
   
    char cmd_copy[100];
    strncpy(cmd_copy, command, sizeof(cmd_copy));
    cmd_copy[sizeof(cmd_copy)-1] = '\0';
    
    if (strchr(cmd_copy, '|') != NULL) {
        #define MAX_C 10
        char *cmds[MAX_C];
        int num_cmds = 0;
        char *pipe_token = strtok(cmd_copy, "|");
        while (pipe_token != NULL && num_cmds < MAX_C) {
            cmds[num_cmds++] = remove_null_space(pipe_token);
            pipe_token = strtok(NULL, "|");
        }
        
        int total_pipes = num_cmds - 1;
        int pipe_fd[2 * total_pipes];
        for (int i = 0; i < total_pipes; i++) {
            if (pipe(pipe_fd + i * 2) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < num_cmds; i++) {
            pid_t pid_pipe = fork();
            if (pid_pipe < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if (pid_pipe == 0) { 
                if (i != 0) {
                    if (dup2(pipe_fd[(i - 1) * 2], STDIN_FILENO) < 0) {
                        perror("dup2 stdin");
                        exit(EXIT_FAILURE);
                    }
                }

                if (i != num_cmds - 1) {
                    if (dup2(pipe_fd[i * 2 + 1], STDOUT_FILENO) < 0) {
                        perror("dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }

                for (int j = 0; j < 2 * total_pipes; j++) {
                    close(pipe_fd[j]);
                }

                char *redir_in = NULL;
                char *redir_out = NULL;
                int append_mode = 0;
                char *segment = cmds[i];
                char *append_pos = strstr(segment, ">>");
                if (append_pos) {
                    *append_pos = '\0';
                    redir_out = append_pos + 2;
                    append_mode = 1;
                } else {
                    char *out_pos = strchr(segment, '>');
                    if (out_pos) {
                        *out_pos = '\0';
                        redir_out = out_pos + 1;
                    }
                }
                char *in_pos = strchr(segment, '<');
                if (in_pos) {
                    *in_pos = '\0';
                    redir_in = in_pos + 1;
                }
                if (redir_out) redir_out = strtok(redir_out, " \t");
                if (redir_in) redir_in = strtok(redir_in, " \t");
                
                if (redir_in) {
                    int fd_in = open(redir_in, O_RDONLY);
                    if (fd_in < 0) {
                        perror("open input");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }
                if (redir_out) {
                    int fd_out;
                    if (append_mode)
                        fd_out = open(redir_out, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    else
                        fd_out = open(redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out < 0) {
                        perror("open output");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                char *args[20];
                int argument_index = 0;
                char *parts = strtok(segment, " \t");
                while (parts != NULL && argument_index < 19) {
                    args[argument_index++] = parts;
                    parts = strtok(NULL, " \t");
                }
                args[argument_index] = NULL;
                if (args[0] == NULL) exit(EXIT_SUCCESS);
                if (execvp(args[0], args) < 0) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }
        }

        for (int i = 0; i < 2 * total_pipes; i++) {
            close(pipe_fd[i]);
        }
        for (int i = 0; i < num_cmds; i++) {
            wait(NULL);
        }
        return 0;
    }

    char *redir_in = NULL;
    char *redir_out = NULL;
    int append_mode = 0;
    char *temp_cmd = cmd_copy;
    char *append_pos = strstr(temp_cmd, ">>");
    if (append_pos) {
        *append_pos = '\0';
        redir_out = append_pos + 2;
        append_mode = 1;
    } else {
        char *out_pos = strchr(temp_cmd, '>');
        if (out_pos) {
            *out_pos = '\0';
            redir_out = out_pos + 1;
        }
    }
    char *in_pos = strchr(temp_cmd, '<');
    if (in_pos) {
        *in_pos = '\0';
        redir_in = in_pos + 1;
    }
    if (redir_out) redir_out = strtok(redir_out, " \t");
    if (redir_in) redir_in = strtok(redir_in, " \t");
    
    char *input[10]; 
    int i = 0;
    char *parts = strtok(temp_cmd, " ");
    while (parts != NULL && i < 9) {
        input[i++] = parts;
        parts = strtok(NULL, " ");
    }
    input[i] = NULL;

    if (input[0] && strcmp(input[0], "cd") == 0) {
        char cwd[1024];
        if (input[1] == NULL) {
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                char *last_slash = strrchr(cwd, '/');
                if (last_slash && last_slash != cwd)
                    *last_slash = '\0';
                else
                    strcpy(cwd, "/");
                if (chdir(cwd) != 0) {
                    perror("cd");
                    return 1;
                }
            } else {
                perror("getcwd");
                return 1;
            }
        } else {
            if (chdir(input[1]) != 0) {
                perror("cd");
                return 1;
            }
        }
        return 0;
    }

    pid = fork();
    if (pid < 0) {
        perror("Fork Failed");
        return 1;
    }
    if (pid == 0) {
        if (redir_out) {
            int fd;
            if (append_mode)
                fd = open(redir_out, O_WRONLY | O_CREAT | O_APPEND, 0644);
            else
                fd = open(redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open output");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (redir_in) {
            int fd = open(redir_in, O_RDONLY);
            if (fd < 0) {
                perror("open input");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        
        if (strcmp(input[0], "pwd") == 0) {
            execl("/bin/pwd", "pwd", NULL);
            perror("execl failed");
            exit(1);
        } else if (strcmp(input[0], "uname") == 0) {
            execl("/bin/uname", "uname", NULL);
            perror("uname failed");
            exit(1);
        } else if (strcmp(input[0], "whoami") == 0) {
            execl("/bin/whoami", "whoami", NULL);
            perror("whoami failed");
            exit(1);
            
        } else {
            execvp(input[0], input);
            perror("execvp");
            exit(1);
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
        pid = -1;
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        else
            return 1;
    }
}

int main(int argc, char *argv[]) {
    int flag = 0;
    char str[100];
    signal(SIGINT, handle_sigint);
    const char *history = "history.txt";

    while (1) {
        char location[1024];
        if (getcwd(location, sizeof(location)) != NULL) {
            printf("%s sh> ", location);
        }
        if (fgets(str, sizeof(str), stdin) == NULL) {
            break; 
        }
        str[strcspn(str, "\n")] = 0;
        if (strcmp(str, "^C") == 0) {
            break;
        }

        if (strcmp(str, "exit") == 0) {
            printf("Exiting shell...\n");
            break;
        }
        
        FILE *fp = fopen(history, "a");
        if (fp == NULL) {
            perror("history file open");
        } else {
            fprintf(fp, "%s\n", str);
            fclose(fp);
        }
        if (strcmp(str, "history") == 0) {
            FILE *fp = fopen(history, "r");
            if (fp == NULL) {
                printf("No history available\n");
                continue;
            }
            char line[100];
            int line_num = 1;
            while (fgets(line, sizeof(line), fp) != NULL) {
                line[strcspn(line, "\n")] = 0;
                printf("%4d  %s\n", line_num++, line);
            }
            fclose(fp);
            continue;
        }

        #define MAX_SEGMENTS 20
        CommandSegment segments[MAX_SEGMENTS];
        int segCount = parse_command_line(str, segments, MAX_SEGMENTS);
        int overall_status = 0;
        for (int i = 0; i < segCount; i++) {

            overall_status = command_processing(segments[i].cmd);

            if (segments[i].op == OP_AND && overall_status != 0) {
                break;
            }
        }
    }
    return 0;
}
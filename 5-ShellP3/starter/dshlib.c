#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"

int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        if (cmd_buff[0] == '\0') {
            continue;
        }

        int res = build_cmd_list(cmd_buff, &clist);
        if (res == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
            continue;
        }
        if (res == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }

        res = execute_pipeline(&clist);
        if (res == OK_EXIT) {
            break;
        }
    }
    return OK;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    char *token = strtok(cmd_line, PIPE_STRING);
    int cmd_count = 0;

    while (token != NULL && cmd_count < CMD_MAX) {
        cmd_buff_t cmd_buff;
        build_cmd_buff(token, &cmd_buff);
        clist->commands[cmd_count++] = cmd_buff;
        token = strtok(NULL, PIPE_STRING);
    }

    if (cmd_count == 0) {
        return WARN_NO_CMDS;
    }

    clist->num = cmd_count;
    return OK;
}

int execute_pipeline(command_list_t *clist) {
    int i;
    int fd[2];
    int prev_fd = 0;

    for (i = 0; i < clist->num; i++) {
        pipe(fd);
        pid_t pid = fork();

        if (pid == 0) {
            if (prev_fd != 0) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            if (i < clist->num - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }

            close(fd[0]);
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            printf("Command execution failed\n");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            close(fd[1]);
            prev_fd = fd[0];
        } else {
            perror("fork failed");
            return ERR_MEMORY;
        }
    }

    for (i = 0; i < clist->num; i++) {
        wait(NULL);
    }

    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    char *token = strtok(cmd_line, " ");
    cmd_buff->argc = 0;
    while (token != NULL) {
        cmd_buff->argv[cmd_buff->argc++] = token;
        token = strtok(NULL, " ");
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    free(cmd_buff->_cmd_buffer);
    return OK;
}

int close_cmd_buff(cmd_buff_t *cmd_buff) {
    return OK;
}

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer) {
        return ERR_MEMORY;
    }
    return OK;
}

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    return BI_NOT_BI;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (match_command(cmd->argv[0]) == BI_CMD_EXIT) {
        return BI_CMD_EXIT;
    }
    return BI_NOT_BI;
}

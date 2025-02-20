#include "dshlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

// EXTRA CREDIT - print the Drexel dragon from the readme.md
extern void print_dragon();

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer) {
        return ERR_MEMORY;
    }
    memset(cmd_buff->_cmd_buffer, 0, SH_CMD_MAX);
    cmd_buff->argc = 0;
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
    }
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    memset(cmd_buff->_cmd_buffer, 0, SH_CMD_MAX);
    cmd_buff->argc = 0;
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    char *token;
    int argc = 0;

    token = strtok(cmd_line, " ");
    while (token != NULL) {
        cmd_buff->argv[argc++] = token;
        if (argc >= CMD_ARGV_MAX) {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        token = strtok(NULL, " ");
    }

    cmd_buff->argv[argc] = NULL; 
    cmd_buff->argc = argc;
    return OK;
}

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    } else if (strncmp(input, "cd", 2) == 0) {
        return BI_CMD_CD;
    } else {
        return BI_NOT_BI;
    }
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd_buff) {
    if (cmd_buff->argc == 1) {
        return BI_CMD_EXIT;
    }
    return BI_NOT_BI;  
}

int exec_cmd(cmd_buff_t *cmd_buff) {
    Built_In_Cmds cmd_type = match_command(cmd_buff->argv[0]);

    if (cmd_type == BI_CMD_EXIT) {
        return OK_EXIT;
    } else if (cmd_type == BI_CMD_DRAGON) {
        print_dragon();
        return OK;
    } else if (cmd_type == BI_CMD_CD) {
        if (cmd_buff->argc > 1) {
            if (chdir(cmd_buff->argv[1]) == -1) {
                perror("cd failed");
            }
        }
        return OK;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (execvp(cmd_buff->argv[0], cmd_buff->argv) == -1) {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) { 
        wait(NULL);
    } else {
        return ERR_MEMORY;
    }

    return OK;
}

int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    cmd_buff_t cmd;
    int rc;

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        rc = alloc_cmd_buff(&cmd);
        if (rc != OK) {
            printf("Memory allocation failed\n");
            break;
        }

        rc = build_cmd_buff(cmd_buff, &cmd);
        if (rc != OK) {
            printf(CMD_WARN_NO_CMD);
            free_cmd_buff(&cmd);
            continue;
        }

        rc = exec_cmd(&cmd);
        if (rc == OK_EXIT) {
            free_cmd_buff(&cmd);
            break;
        }

        free_cmd_buff(&cmd);
    }

    return OK;
}

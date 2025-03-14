#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (cmd_line == NULL || cmd_buff == NULL) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    char *original_line = strdup(cmd_line);
    if (original_line == NULL) {
        return ERR_MEMORY;
    }

    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = original_line;

    char *ptr = original_line;
    char *arg_start = NULL;
    bool in_quotes = false;//For quotes 
    char quote_char = '\0';

    while (*ptr != '\0') {
        if ((isspace((unsigned char)*ptr) && !in_quotes)) {
            if (arg_start != NULL) {
                *ptr = '\0';
                cmd_buff->argv[cmd_buff->argc++] = arg_start;
                arg_start = NULL;
            }
        } else if (*ptr == '"' || *ptr == '\'') {
            if (in_quotes && *ptr == quote_char) {
                in_quotes = false;
                quote_char = '\0';
                *ptr = '\0';  // End the argument at the closing quote
            } else if (!in_quotes) {
                in_quotes = true;
                quote_char = *ptr;
                arg_start = ptr + 1;  // Start the argument after the opening quote
            }
        } else {
            if (arg_start == NULL) {
                arg_start = ptr;
            }
        }
        ptr++;
    }

    if (arg_start != NULL) {
        cmd_buff->argv[cmd_buff->argc++] = arg_start;
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;

	 // Debugging output to verify parsing
	 // printf("Parsed command:\n");
	 // for (int i = 0; i < cmd_buff->argc; i++) {
	 //     printf("  argv[%d]: %s\n", i, cmd_buff->argv[i]);
	 // }



    return OK;
}

char *trim_whitespace(char *str) { //Had to make a new function for readability
    char *end;

    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // new null terminator
    *(end + 1) = '\0';

    return str;
}




int parse_pipeline(const char *cmd_line, command_list_t *clist) {
    if (cmd_line == NULL || clist == NULL) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    char *line_copy = strdup(cmd_line);
    if (line_copy == NULL) {
        return ERR_MEMORY;
    }
	 //Parsing using pipe.
    clist->num = 0;
    char *saveptr;
    char *command = strtok_r(line_copy, "|", &saveptr);

    while (command != NULL) {
        if (clist->num >= CMD_MAX) {
            free(line_copy);
            return ERR_TOO_MANY_COMMANDS;
        }

        cmd_buff_t *cmd_buff = &clist->commands[clist->num];
        int result = build_cmd_buff(command, cmd_buff);
        if (result != OK) {
            free(line_copy);
            return result;
        }

        clist->num++;
        command = strtok_r(NULL, "|", &saveptr);
    }

    free(line_copy);
    return OK;
}



int execute_pipeline(command_list_t *clist) {
    int num_commands = clist->num;
    int pipefd[2 * (num_commands - 1)];
    pid_t pids[num_commands];
	
	 // for (int i = 0; i < clist->num; i++) {
    //     printf("Command %d:\n", i);
	 //     for (int j = 0; clist->commands[i].argv[j] != NULL; j++) {
	 //         printf("  argv[%d]: %s\n", j, clist->commands[i].argv[j]);
	 //     }
	 // }




    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipefd + 2 * i) == -1) {
            perror("pipe");
            return ERR_MEMORY;
        }
    }

    // Fork processes for each command
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            return ERR_MEMORY;
        }

        if (pids[i] == 0) {  // Child process
            // if not first moves input
            if (i > 0) {
                if (dup2(pipefd[2 * (i - 1)], STDIN_FILENO) == -1) {
                    perror("dup2 stdin");
                    exit(EXIT_FAILURE);
                }
            }

            // Redirect output if not the last command
            if (i < num_commands - 1) {
                if (dup2(pipefd[2 * i + 1], STDOUT_FILENO) == -1) {
                    perror("dup2 stdout");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe file descriptors. Very important stuff
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipefd[j]);
            }

            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    // Closes all pipe
    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipefd[i]);
    }

    // waitpid so everyhting is smooth
    for (int i = 0; i < num_commands; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Command %d exited with status %d\n", i, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Command %d terminated by signal %d\n", i, WTERMSIG(status));
        }
    }

    return OK;
}



int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    int rc = OK;

    while (1) {
        printf("%s", SH_PROMPT);

        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        if (strlen(cmd_buff) == 0) {
            printf("%s\n", CMD_WARN_NO_CMD);
            rc = WARN_NO_CMDS;
            continue;
        }

        if (strstr(cmd_buff, "|") != NULL) {
            command_list_t cmd_list;
            if (parse_pipeline(cmd_buff, &cmd_list) != OK) {
                fprintf(stderr, "%s\n", CMD_ERR_PIPE_LIMIT);
                rc = ERR_TOO_MANY_COMMANDS;
                continue;
            }

            // Execute the pipeline

            execute_pipeline(&cmd_list);

            // Free memory for each command's buffer
            for (int i = 0; i < cmd_list.num; i++) {
                free(cmd_list.commands[i]._cmd_buffer);
            }
        } else {
            cmd_buff_t cmd;
            if (build_cmd_buff(cmd_buff, &cmd) != OK) {
                fprintf(stderr, "%s\n", CMD_ERR_PIPE_LIMIT);
                rc = ERR_MEMORY;
                continue;
            }

            if (strcmp(cmd.argv[0], EXIT_CMD) == 0) {
                free(cmd._cmd_buffer);
                break;
            }

            if (strcmp(cmd.argv[0], "cd") == 0) {
                if (cmd.argc == 1) {
                    chdir(getenv("HOME"));
                } else if (cmd.argc == 2) {
                    if (chdir(cmd.argv[1]) != 0) {
                        perror("cd");
                    }
                } else {
                    fprintf(stderr, "%s\n", CMD_ERR_PIPE_LIMIT);
                }
                free(cmd._cmd_buffer);
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                rc = ERR_MEMORY;
            } else if (pid == 0) {
                execvp(cmd.argv[0], cmd.argv);
                perror("execvp failed");
                exit(EXIT_FAILURE);
            } else {
                int status;
                wait(&status);

                if (WIFEXITED(status)) {
                    if (WEXITSTATUS(status) != 0) {
                        fprintf(stderr, "Command failed with exit code %d\n", WEXITSTATUS(status));
                    }
                }
            }

            free(cmd._cmd_buffer);
        }
    }

    return rc;
}




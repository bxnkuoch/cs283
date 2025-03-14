// Extra credit in this assignment
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>


#include <signal.h>
#include <pthread.h>


#include "dshlib.h"
#include "rshlib.h"


/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 * 
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port  
 * 
 *      is_threded:  Used for extra credit to indicate the server should implement
 *                   per thread connections for clients  
 * 
 *      This function basically runs the server by: 
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server. 
 * 
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.  
 * 
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.  
 */

int start_server(char *ifaces, int port, int is_threaded) {
    (void)is_threaded;
    int svr_socket;
    int rc;

    printf("SERVER: Starting server on %s:%d...\n", ifaces, port);

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        int err_code = svr_socket;
        printf("SERVER: Failed to boot server."); 
        return err_code;
    }

    rc = process_cli_requests(svr_socket);

    printf("SERVER: Stopping server...\n");
    stop_server(svr_socket);

    return rc;
}




/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function. 
 * 
 *      This function simply returns the value of close() when closing
 *      the socket.  
 */

int stop_server(int svr_socket) {
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.   
 * 
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically: 
 * 
 *      1. Create the server socket using the socket() function. 
 *      2. Calling bind to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 * 
 *      after creating the socket and prior to calling bind you might want to 
 *      include the following code:
 * 
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 * 
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 * 
 *  Returns:
 * 
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor, 
 *                      which is just an integer.
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails. 
 * 
 */

int boot_server(char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;

    printf("SERVER: Booting server on %s:%d...\n", ifaces, port);

    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket == -1) {
        perror("SERVER: socket failed");
        return ERR_RDSH_SERVER;
    }

    int enable = 1;
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        close(svr_socket);
        return ERR_RDSH_SERVER;
    }

    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        //perror("SERVER: inet_pton failed");
        close(svr_socket);
        return ERR_RDSH_SERVER;
    }
    addr.sin_port = htons(port);

    ret = bind(svr_socket, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        close(svr_socket);
        return ERR_RDSH_SERVER;
    }
    printf("SERVER: Socket bound to address.\n");

    ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("SERVER: listen failed");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }
    printf("SERVER: Listening for connections...\n");

    printf("SERVER: Server booted successfully on %s:%d\n", ifaces, port);
    return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *   
 *  This function handles managing client connections.  It does this using
 *  the following logic
 * 
 *      1.  Starts a while(1) loop:
 *  
 *          a. Calls accept() to wait for a client connection. Recall that 
 *             the accept() function returns another socket specifically
 *             bound to a client connection. 
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.  
 * 
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop. 
 * 
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to 
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket. 
 * 
 *  Returns:
 * 
 *      OK_EXIT:  When the client sends the `stop-server` command this function
 *                should return OK_EXIT. 
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept() 
 *                function failing. 
 * 
 *      OTHERS:   See exec_client_requests() for return codes.  Note that positive
 *                values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server. 
 * 
 */


int process_cli_requests(int svr_socket) {
    int cli_socket;
    pthread_t thread_id;
    void *thread_ret;

    printf("SERVER: Waiting for client connections...\n");

    while (1) {
        printf("SERVER: Waiting for a client to connect...\n");

        cli_socket = accept(svr_socket, NULL, NULL);
        if (cli_socket == -1) {
            perror("SERVER: accept failed");
            return ERR_RDSH_COMMUNICATION;
        }
        printf("SERVER: Client connected. Socket: %d\n", cli_socket);

        if (pthread_create(&thread_id, NULL, exec_client_requests, (void *)&cli_socket) < 0) {
            perror("SERVER: Failed to create thread");
            close(cli_socket);
            return ERR_RDSH_COMMUNICATION;
        }

        // Wait for the thread to finish and check its return value
        pthread_join(thread_id, &thread_ret);
        if (thread_ret == (void *)OK_EXIT) {
            printf("SERVER: Received stop-server command. Shutting down...\n");
            break;  // Exit the loop and shut down the server
        }
    }

    return OK;
}

/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */

void *exec_client_requests(void *socket_handle) {
    int cli_socket = *((int *)socket_handle);
    char cmd_buff[RDSH_COMM_BUFF_SZ];
    ssize_t recv_bytes;

    printf("SERVER: Handling client request. Socket: %d\n", cli_socket);

    recv_bytes = recv(cli_socket, cmd_buff, sizeof(cmd_buff), 0);
    if (recv_bytes <= 0) {
        perror("SERVER: recv failed");
        close(cli_socket);
        pthread_exit(NULL);
    }
    cmd_buff[recv_bytes] = '\0';
    printf("SERVER: Received command: %s\n", cmd_buff);

    // Check for stop-server command
    if (strcmp(cmd_buff, "stop-server") == 0) {
        printf("SERVER: Received stop-server command. Shutting down...\n");
        close(cli_socket);
        pthread_exit((void *)OK_EXIT);  // Signal server to stop
    }

    // Execute the command
    if (strstr(cmd_buff, "|") != NULL) {
        // Handle pipeline commands
        command_list_t cmd_list;
        if (parse_pipeline(cmd_buff, &cmd_list) != OK) {
            fprintf(stderr, "SERVER: Error parsing pipeline\n");
            close(cli_socket);
            pthread_exit(NULL);
        }

        rsh_execute_pipeline(cli_socket, &cmd_list);

        for (int i = 0; i < cmd_list.num; i++) {
            free(cmd_list.commands[i]._cmd_buffer);
        }
    } else {
        // Handle single commands
        cmd_buff_t cmd;
        if (build_cmd_buff(cmd_buff, &cmd) != OK) {
            fprintf(stderr, "SERVER: Error building command buffer\n");
            close(cli_socket);
            pthread_exit(NULL);
        }

        if (strcmp(cmd.argv[0], EXIT_CMD) == 0) {
            printf("SERVER: Client requested exit.\n");
            free(cmd._cmd_buffer);
            close(cli_socket);
            pthread_exit(NULL);
        } else if (strcmp(cmd.argv[0], "cd") == 0) {
            // Handle cd command
            if (cmd.argc == 1) {
                chdir(getenv("HOME"));
            } else if (cmd.argc == 2) {
                if (chdir(cmd.argv[1]) != 0) {
                    perror("SERVER: cd failed");
                }
            } else {
                fprintf(stderr, "SERVER: cd: too many arguments\n");
            }
            free(cmd._cmd_buffer);
            close(cli_socket);
            pthread_exit(NULL);
        }

        // Fork and execute the command
        pid_t pid = fork();
        if (pid < 0) {
            perror("SERVER: fork failed");
            close(cli_socket);
            pthread_exit(NULL);
        } else if (pid == 0) {
            // Child process: execute the command
            dup2(cli_socket, STDOUT_FILENO);
            dup2(cli_socket, STDERR_FILENO);
            execvp(cmd.argv[0], cmd.argv);
            perror("SERVER: execvp failed");
            exit(EXIT_FAILURE);
        } else {
            // Parent process: wait for the child to finish
            int status;
            wait(&status);

            if (WIFEXITED(status)) {
                printf("SERVER: Command executed with exit code: %d\n", WEXITSTATUS(status));
            }
        }

        free(cmd._cmd_buffer);
    }

    // Send EOF to indicate end of response
    send_message_eof(cli_socket);

    printf("SERVER: Client socket closed properly.\n");
    close(cli_socket);
    pthread_exit(NULL);
}


/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */

int send_message_eof(int cli_socket) {
    printf("SERVER: Sending EOF to client. Socket: %d\n", cli_socket);
    if (send(cli_socket, &RDSH_EOF_CHAR, 1, 0) < 0) {
        perror("SERVER: send failed");
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}



/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */

int send_message_string(int cli_socket, char *buff) {
    printf("SERVER: Sending message to client. Socket: %d\n", cli_socket);
    int bytes_sent = send(cli_socket, buff, strlen(buff), 0);
    if (bytes_sent < 0) {
        perror("SERVER: send failed");
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */


 int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int pipes[clist->num - 1][2];  // Array of pipes
    pid_t pids[clist->num];
    int pids_st[clist->num];       // Array to store process statuses
    int exit_code;

    printf("SERVER: Executing pipeline with %d commands.\n", clist->num);

    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("SERVER: pipe failed");
            return ERR_RDSH_COMMUNICATION;
        }
        printf("SERVER: Created pipe %d.\n", i);
    }

    // Fork and execute each command in the pipeline
    for (int i = 0; i < clist->num; i++) {
        printf("SERVER: Forking for command %d: %s\n", i, clist->commands[i].argv[0]);
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("SERVER: fork failed");
            return ERR_RDSH_COMMUNICATION;
        }

        if (pids[i] == 0) {  // Child process
            printf("SERVER: Child process %d executing command: %s\n", i, clist->commands[i].argv[0]);
            if (i == 0) {
                if (dup2(cli_sock, STDIN_FILENO) == -1) {
                    perror("SERVER: dup2 stdin failed");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Redirect stdin to the read end of the previous pipe
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("SERVER: dup2 stdin failed");
                    exit(EXIT_FAILURE);
                }
            }

            if (i == clist->num - 1) {
                if (dup2(cli_sock, STDOUT_FILENO) == -1) {
                    perror("SERVER: dup2 stdout failed");
                    exit(EXIT_FAILURE);
                }
                if (dup2(cli_sock, STDERR_FILENO) == -1) {
                    perror("SERVER: dup2 stderr failed");
                    exit(EXIT_FAILURE);
                }
            } else {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("SERVER: dup2 stdout failed");
                    exit(EXIT_FAILURE);
                }
            }

            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("SERVER: execvp failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &pids_st[i], 0);
    }

    exit_code = WEXITSTATUS(pids_st[clist->num - 1]);
    for (int i = 0; i < clist->num; i++) {
        if (WEXITSTATUS(pids_st[i]) == EXIT_SC) {
            exit_code = EXIT_SC;
            break;
        }
    }

    printf("SERVER: Pipeline execution complete. Exit code: %d\n", exit_code);
    return exit_code;
}

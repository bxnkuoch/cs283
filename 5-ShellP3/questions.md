1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

_The wait() system call is used after forking each child process to ensure that the parent waits for all child processes to finish before proceeding. If waitpid() or wait() were not there, the shell could continue accepting new input and executing commands before the previous child processes had finished. This could cause unexpected behavior._

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

_The unused pipe ends must be closed to prevent file descriptor leakage. If they are left open, these file descriptors would still be active which could use up resources and potentially causing deadlocks._

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

_This is because cd directly modifies the shell's working directory. If cd were external, the child process would change the directory only within its own environment, and the parent shell would remain unaffected._

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

_Dynamic memory allocation could be used for the command list which would allocate space as needed during input parsing. However, this woudl also have trade-offs such as increased complexity in memory management and potential performance overhead if the system has to frequently resize memory buffers during execution._

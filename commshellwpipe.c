#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include "gettext.h"

#define COMMAND_BUF_SIZE 1024
#define MAX_NUM_ARGS 3 //200
#define MAX_NUM_EXEC 2

int parsing_command(
        char *str,
        char **const args,
        size_t num_args);


int parsing_set_for_exec(
        char *const str,
        char ***for_exec,
        size_t const max_num_exec,
        size_t *num_exec,
        char **args,
        size_t num_args);


void free_mem_for_exec(
        char ***for_exec,
        size_t *num_exec);


int exec_progs(
        char ***for_exec,
        size_t *num_exec,
        bool *child);

int execute_shell(
        size_t curr_exec,
        char ***for_exec,
        size_t *num_exec,
        bool *child,
        int cond_pipes,
        int *pipefd);

int waiting_shell();


int main()
{
    int ret = 0;
    time_t curr_time;
    bool child = false;

    char command_buf[COMMAND_BUF_SIZE];
    char *args[MAX_NUM_ARGS + 2] = {NULL};

    char **for_exec[MAX_NUM_EXEC];
    size_t num_exec = 0;

    curr_time = time(NULL);
    printf("commandshell 0.1 %s\n", ctime(&curr_time));
    puts("To exit the shell, enter \"exit\", \"Exit\" ...");

    for(;;)
    {
        printf("command> ");
        ret = get_text(
                command_buf,
                sizeof(command_buf));
        if (EOF == ret )
        {
            break;
        }
        else if (EXIT_FAILURE == ret)
        {
            puts("Input error!");
        }
        else if (0 == ret)
        {
            ret = parsing_set_for_exec(
                    command_buf,
                    for_exec,
                    sizeof(for_exec) / sizeof(for_exec[0]),
                    &num_exec,
                    args,
                    sizeof(args) / sizeof(args[0]));

            puts("after parsing_num_exec");

            printf("ret = %d   num_exec = %ld\n", ret, num_exec);

            if (-1 == ret)
            {
                break;
            }
            else if (1 == ret)
            {
                puts("Error in parsing sets for exec!");
            }
            else if (0 == strncasecmp(*for_exec[0], "exit", 5))
            {
                puts("Exiting the commandshell");
                break;
            }

            ret = exec_progs(
                    for_exec,
                    &num_exec,
                    &child);

            printf("exec_prog ret = %d\n", ret);
            if (true == child || 0 != ret)
            {
                break;
            }

            free_mem_for_exec(
                    for_exec, 
                    &num_exec);
        }
    }

    if (0 != num_exec)// && !child)
    {
        free_mem_for_exec(
                for_exec,
                &num_exec);
    }

    return ret;
}


int parsing_command(
        char *const str,
        char **const args,
        size_t const num_args)
{
    int ret = 0;
    size_t i = 0;
    char *pstr = NULL;
    char *save_ptr_args = NULL;

    printf("str = %p\n", str);

    pstr = strtok_r(str, " ", &save_ptr_args);
    if (NULL == pstr)
    {
        fprintf(stderr, "Error! The command line is empty or has no subdelim: %s\n\n",
                str);
        ret = EXIT_FAILURE;
        goto finally;
    }

    args[i] = pstr;
    printf("args[%ld] = %s\n", i, args[i]);

    i++;

    while (NULL != (pstr = strtok_r(NULL, " ", &save_ptr_args)) && i < (num_args - 1))
    {
        args[i] = pstr;
         printf("args[%ld] = %s\n", i, args[i]);

        i++;
    }

    if (NULL != pstr && (num_args - 1) == i)
    {
        fprintf(stderr, "Too much arguments! The number of arguments must not exceed"
                " %ld\n", num_args - 2);
    }

    args[i] = NULL;
    printf("args[%ld] = %s\n", i, args[i]);

 finally:

    return ret;
}


int parsing_set_for_exec(
        char *const str,
        char ***for_exec,
        size_t const max_num_exec,
        size_t *const num_exec,
        char **args,
        size_t num_args)
{
    int ret = 0;
    size_t i = 0;

    char *pstr_tmp = str;
    char *pstr = NULL;
    char *save_ptr_exec = NULL;

    *num_exec = 0;

    while (NULL != (pstr = strtok_r(pstr_tmp, "|", &save_ptr_exec)) && i < max_num_exec)
    {
        printf("\n\nin while\n");

        for_exec[i] = calloc(num_args, sizeof(char *));
        if (NULL == for_exec[i])
        {
            perror("Error in calloc(...), function parsing_set_for_exec(...)");
            ret = -1;
            goto finally;
        }

        printf("token_exec = %p    i = %ld\n", pstr, i);

        parsing_command(pstr, for_exec[i], num_args);

        puts("after parsing_command");

        pstr_tmp = NULL;
        i++;
    }

    if (0 == i)
    {
        fprintf(stderr, "Error! The command line is empty or has no pipe token \"|\":"
                " %s\n\n", str);
        ret = EXIT_FAILURE;
    }
    else if (NULL != pstr &&  max_num_exec == i)
    {
        fprintf(stderr, "Too much pipes! The number of pipe tokens \"|\" should not" 
                " exceed %ld\n", max_num_exec - 1);
    }

 finally:

    if (-1 == ret && 0 != i)
    {
        free_mem_for_exec(for_exec, &i);
    }
    else
    {
        *num_exec = i;
    }

    return ret;
}


void free_mem_for_exec(
        char ***for_exec,
        size_t *num_exec)
{
    for (size_t i = 0; i < *num_exec; i++)
    {
        if ( NULL != for_exec[i])
        {
            free(for_exec[i]);
            for_exec[i] = NULL;
        }
    }

    *num_exec = 0;
}


typedef enum Pipe_conditions
{
    COND_NOT_PIPE = -1,
    COND_WITH_PIPE = 1,
} Pipe_conditions;


void close_pipes(int pipe_in, int pipe_out)
{
    if (pipe_in >= 0)
    {
        close (pipe_in);
    }
    if (pipe_out >= 0)
    {
        close (pipe_out);
    }
}

int exec_progs(
        char ***for_exec,
        size_t *num_exec,
        bool *child)
{
    int ret = 0;
    size_t i = 0;


    *child = false;
    int cond_pipes;

    static int pipefd[2] = {0 , 0};

    if (*num_exec <= 0)
    {
        fprintf(stderr, "No exec progs, num exec = %ld\n", *num_exec);
        ret = EXIT_FAILURE;
        goto finally;
    }
    else if (1 == *num_exec)
    {
        cond_pipes = COND_NOT_PIPE;
    }
    else
    {
        cond_pipes = COND_WITH_PIPE;
    }

    for (i = 0; i < *num_exec; i++)
    {


    if (COND_WITH_PIPE == cond_pipes && 0 == i)
    {
        puts("create pipe pipe(pipefd)");
        if (-1 == pipe(pipefd))
        {
            perror("Error in pipe(...), function exec_progs(...)");
            ret = -1;
            break;
        }
    }



        ret = execute_shell(
                i,
                for_exec,
                num_exec,
                child,
                cond_pipes,
                pipefd);

/*

        switch (cond_pipes)
        {
            case COND_WITHOUT_PIPES:
            {
                execute_shell();
                break;
            }
            case COND_WITH_PIPES:
            {

                break;
            }
            default:
            {
                ret = -1;
                break;
            }
        }
*/        
        if (0 != ret || *child)
        {
            break;
        }
    }

    if (0 != ret && 0 == i)
    {
        goto finally;
    }

    if (!(*child))
    {
        puts("Before close_pipes()");
if (COND_WITH_PIPE == cond_pipes)
{
    close_pipes(pipefd[0], pipefd[1]);
}
    printf("before waiting ret = %d\n", ret);
    ret = waiting_shell();
    printf("after waiting ret = %d\n", ret);
    }

 finally:

    return ret;
}

typedef enum Pipe_state
{
    STATE_OUT_PIPE = 0,
    STATE_TWO_PIPES,
    STATE_IN_PIPE,
} Pipe_state;


int dup_pipes(int pipe_in, int pipe_out)
{
    int ret = 0;

    if (pipe_out > COND_NOT_PIPE)
    {
        if (-1 == dup2(pipe_out, 1))
        {
            fprintf(stderr, "Error in pipe_out dup2(%d, %d)\n", pipe_out, 1);
            ret = -1;
        }
	    if (1 != pipe_out)
        {
            close (pipe_out);
        }
    }

    if (pipe_in > COND_NOT_PIPE)
    {
        if (-1 == dup2(pipe_in, 0))
        {
            fprintf(stderr, "Error in pipe_in dup2(%d, %d)\n", pipe_in, 0);
            ret = -1;
        }
        if (0 != pipe_in)
        {
            close (pipe_in);
        }
    }

    return ret;
}

int execute_shell(
        size_t curr_exec,
        char ***for_exec,
        size_t *num_exec,
        bool *child,
        int cond_pipes,
        int *pipefd)
{
    int ret = 0;
    pid_t pid;

    *child = false;
/*
    static int pipefd[2] = {0 , 0};

    if (COND_WITH_PIPE == cond_pipes && 0 == curr_exec)
    {
        puts("create pipe pipe(pipefd)");
        if (-1 == pipe(pipefd))
        {
            perror("Error in pipe(...), function exec_progs(...)");
            ret = -1;
            goto finally;
        }
    }

    printf("curr_exec = %ld, pipefd[0] = %d, pipefd[1] = %d\n", curr_exec, pipefd[0], pipefd[1]);
*/
    errno = 0;
    switch (pid = fork())
    {
        case -1: // Error in fork()
        {
            //*child = false;

            perror("Error in fork()");
            ret = -1;
            break;
        }
        case 0: // Child process
        {
            *child = true;
            
            if (COND_WITH_PIPE == cond_pipes && 0 == curr_exec)
            {
                close(pipefd[0]);
                dup_pipes(COND_NOT_PIPE, pipefd[1]);
            }
            else if (COND_WITH_PIPE == cond_pipes && 1 == curr_exec)
            {
                close(pipefd[1]);
                dup_pipes(pipefd[0], COND_NOT_PIPE);
            }
            /*  
            if (COND_WITH_PIPE == cond_pipes)
            {
            //close_pipes(pipe_in, pipe_out);
            dup_pipes(pipe_in, pipe_out);
            }
            */

            sleep(2);

            errno = 0;
            if (-1 == execvp(for_exec[curr_exec][0], for_exec[curr_exec]) && 0 != errno)
            {
                ret = errno;
                perror("Error in command");
            }
            else
            {
                ret = -1;
            }
            break;
        }
        default: // Parent process
        {
            //*child = false;
            break;
        }
    }

 finally:

    return ret;
}

int waiting_shell()
{
    int wstatus;
    int ret = 0;
    pid_t pid;
    size_t i = 1;

    errno = 0;

    while ((pid = waitpid(-1, &wstatus, 0)) > 0)
    {
    printf("in while ret = %d i = %ld\n", ret, i);
        if (WIFEXITED(wstatus))
        {
            printf("Shell %ld returned status: %d\n", i, WEXITSTATUS(wstatus));
        }
        else
        {
            printf("Shell %ld aborted/interrupded with status: %d\n", i, wstatus);
        }
        i++;
        errno = 0;
    }

    printf("after while  ret = %d i = %ld\n", ret, i);

    if (ECHILD != errno)
    {
        perror("Error waitpid(...)");
        ret = -1;
    }

    return ret;
}

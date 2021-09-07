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


int exec_prog(
        char **args,
        bool *child);


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


/*
            ret = exec_prog(args, &child);
            if (true == child || 0 != ret)
            {
                break;
            }*/

            free_mem_for_exec(
                    for_exec, 
                    &num_exec);
        }
    }

    if (0  != num_exec)
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


int exec_prog(
        char **args,
        bool *const child)
{
    int ret = 0;

    pid_t pid;
    int wstatus = 0;

    errno = 0;
    switch (pid = fork())
    {
        case -1: // Error in fork()
        {
            *child = false;

            perror("Error in fork()");
            ret = -1;
            break;
        }
        case 0: // Child process
        {
            *child = true;

            errno = 0;
            if (-1 == execvp(args[0], args) && 0 != errno)
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
            *child = false;

            if (-1 == waitpid(pid, &wstatus, 0))
            {
                perror("Error in wait(...)");
                ret = -1;
                goto finally;
            }

            if (WIFEXITED(wstatus))
            {
                printf("Shell returned status: %d\n", WEXITSTATUS(wstatus));
            }
            else
            {
                printf("Shell aborted/interrupded with status: %d\n", wstatus);
            }

            break;
        }
    }

 finally:

    return ret;
}

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "gettext.h"

// Ввод текста
int get_text(
        char *const str,
        size_t const str_size)
{
    int ret = 0;
    int n = 0;
    char *pstr = NULL;
    
    errno = 0;
    n = scanf("%m[^\n]", &pstr);

    if (1 == n)
    {
        if (snprintf(str, str_size, "%s", pstr) < 0)
        {
            ret = EXIT_FAILURE;
        }

        free(pstr);
    }
    else if (0 != errno)
    {
        perror("Error in scanf(...) get_text(..)");
        ret = EXIT_FAILURE;
    }
    else
    {
        ret = EXIT_FAILURE;
    }

    if (EOF == clean_stdin())
    {
        ret = EOF;
        goto finally;
    }

 finally:

    return ret;
}

// Очистка stdin
int clean_stdin()
{
    int ret = 0;
    int ch_trash;

    do
    {
        ch_trash = getchar();

    } while ('\n' != ch_trash && EOF != ch_trash);

    if (EOF == ch_trash)
    {
        puts("\nEOF is entered in stdin!");
        ret = EOF;
    }

    return ret;
}

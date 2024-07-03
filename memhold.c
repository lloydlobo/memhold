// file: main.c

#include "memhold.h" // Declares module functions

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

// TOP

typedef struct Memhold_API
{
    const char *id;
} Memhold_API;

MHAPI void InitMemhold(void);

int main(int argc, char *argv[])
{

    Memhold_API memholdApi = {.id = "memhold"};

    memholdApi.id = "memhold";

    printf("memholdApi.id = %s\n", memholdApi.id);

    {
        int foo = 0;
        printf("foo = %d\n", foo);
        return foo;
    }

    return 0;
}

// BOT

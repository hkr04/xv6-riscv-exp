#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc > 1) {
        fprintf(2, "No need for parameters\n");
        exit(1);
    }

    test_heap();

    exit(0);
}
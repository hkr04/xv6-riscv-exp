#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    uint mask;

    // attempt to parse mask
    if (argc < 3 || (mask = atoi(argv[1])) <= 0) {
        fprintf(2, "Usage: %s <mask> <command>\n", argv[0]);
        exit(1);
    }

    // attempt to set trace mask
    if (trace(mask) < 0) {
        fprintf(2, "%s: trace failed\n", argv[0]);
        exit(1);
    }

    // execute the command that needs to be traced
    if (exec(argv[2], &argv[2]) < 0) { 
        fprintf(2, "Execution failed\n");
        exit(1);
    }

    exit(0);
}
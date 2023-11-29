#include "processdumper.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int debug = 0;
FILE *outfile = NULL;
char *filename;

int main(int argc, char *argv[]) {
    // Killswitch
    // Use :r! echo $(($(date +\%s)+604800))
    // Where 86400 is to make binary valid 1 day, 604800 is 1 week
    
    u_int64 endTime = 0000000000;
    u_int64 now = time(NULL);
    if (now > endTime) {
        return 0;
    }

    if (argc < 3) {
        printf("Usage: %s [options] <process name> <filepath>\n", argv[0]);
        printf("");
        printf("  [options]\n");
        printf("    -d, enable debug outprints\n");
        printf("\n");
        return -1;
    }
    char *targetProcess;
    if (argc > 3) {
        if (strncmp(argv[1], "-d", 2) == 0) {
            debug = 1;
        } else {
            printf("Unknown parameter: %s\n", argv[1]);
            return -1;
        }
        targetProcess = argv[2];
        filename = argv[3];
    } else {
        targetProcess = argv[1];
        filename = argv[2];
    }
    if (debug) {
        printf("[Debug] Attempting to open file (%s) for writing\n", filename);
    }

    if ((outfile = fopen(filename, "wb")) == NULL) // will replace existing files
    {
        printf("Failed to open file (%s)\n", filename);
        return -1;
    }

    //DumpProcess(debug, targetProcess, filename, outfile);
    DumpProcess(targetProcess);
}

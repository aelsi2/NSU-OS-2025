#include "io.h"
#include "parse.h"

int main(int argc, char *argv[]) {

    /* PLACE SIGNAL CODE HERE */

    char line[1024];
    while (prompt_line(line, sizeof(line))) { 
        char *ptr = line;
        static pipeline_t pipeline;
        while (parse_pipeline(&ptr, &pipeline)) {

        }
    }
    return 0;
}

/* PLACE SIGNAL CODE HERE */

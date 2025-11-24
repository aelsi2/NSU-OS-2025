#ifndef __PARSE_H
#define __PARSE_H

#define MAXCMD 50
#define MAXARGS 256

typedef struct {
    char *args[MAXARGS];
} command_t;

typedef struct {
    command_t commands[MAXCMD];
    int cmd_count;
    char *in_file;
    char *out_file;
    char flags;
} pipeline_t;

#define PLAPPEND 0x01
#define PLBKGRND 0x02

int parse_pipeline(char **line_ptr, pipeline_t *pipeline);

#endif // __PARSE_H

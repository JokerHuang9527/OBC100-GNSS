/*
 * cmd.h
 *
 *  Created on: 2019¦~10¤ë22¤é
 *      Author: kusoyao
 */

#ifndef INC_CMD_H_
#define INC_CMD_H_

#define MAX_CMD_LEN 256
#define MAX_ARGS 10

typedef int (*cmd_handle_t)(int argc, char **argv);


typedef struct cmd {
    struct cmd *next;
    char *cmd;
    char *helpmsg;
    cmd_handle_t handle;
}cmd_t;

int init_cmd();
int register_cmd(cmd_t *c);
cmd_t * find_cmd(char *name);
int execute_cmd(int argc, char **argv);

enum errorcode
{
    CMD_SUCCESS = 0,
    CMD_FAIL = -1,
    CMD_NOT_FOUND = -2,
    CMD_NOT_IMPLEMENT = -3,
};



#endif /* INC_CMD_H_ */

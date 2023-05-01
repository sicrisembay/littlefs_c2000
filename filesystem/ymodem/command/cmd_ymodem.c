/*!
 * \file cmd_ymodem.c
 */

#include "autoconf.h"
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include "FreeRTOS_CLI.h"
#include "../ymodem.h"

#if CONFIG_ENABLE_CLI_YMODEM_COMMAND

#if CONFIG_ENABLE_CLI_YMODEM_CMD_RECEIVE

static Int16 FuncYmodemRx(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    FILE ffd = {0};
    char name[256] = {0};
    memset(write_buffer, 0, bufferLen);

    int ret = Ymodem_Receive(&ffd, 16384, name);
    if(ret <= 0) {
        System_snprintf(write_buffer, bufferLen,
                "\r\n    Error %d\r\n\r\n", ret);
    } else {
        System_snprintf(write_buffer, bufferLen,
                "\r\n    %s (%d bytes) received\r\n\r\n", name, ret);
    }
    return 0;
}

static const CLI_Command_Definition_t ymodem_cmd_receive = {
    "ymodem_receive",
    "ymodem_receive:\r\n"
    "    Receives file using Y-MODEM\r\n\r\n",
    FuncYmodemRx,
    0
};

#endif


#if CONFIG_ENABLE_CLI_YMODEM_CMD_SEND

static Int16 FuncYmodemSend(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    FILE ffd = {0};
    char * ptrStrParam;
    Int16 strParamLen;

    memset(write_buffer, 0, bufferLen);

    /* Get file name */
    ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
    if(ptrStrParam == NULL) {
        System_snprintf(write_buffer, bufferLen, "    Error: Parameter1 not found!\r\n\r\n");
        return 0;
    }

    int ret = Ymodem_Transmit(ptrStrParam, 16384, &ffd); // dummy size
    if(ret < 0) {
        System_snprintf(write_buffer, bufferLen,
                "\r\n    Error %d\r\n\r\n", ret);
    } else {
        System_snprintf(write_buffer, bufferLen,
                "\r\n    %s sent\r\n\r\n", ptrStrParam);
    }
    return 0;
}

static const CLI_Command_Definition_t ymodem_cmd_send = {
    "ymodem_send",
    "ymodem_send <filename>:\r\n"
    "    Sends <filename> using Y-MODEM\r\n\r\n",
    FuncYmodemSend,
    1
};

#endif


void CMD_YMODEM_init(void)
{
#if CONFIG_ENABLE_CLI_YMODEM_CMD_RECEIVE
    CLIRegisterCommand(&ymodem_cmd_receive);
#endif

#if CONFIG_ENABLE_CLI_YMODEM_CMD_SEND
    CLIRegisterCommand(&ymodem_cmd_send);
#endif
}

#endif /* CONFIG_ENABLE_CLI_YMODEM_COMMAND */

/*!
 * \file cmd_lfs.c
 */

#include "driver_def.h"
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include "FreeRTOS_CLI.h"
#include "../littlefs/lfs.h"
#include "../lfs_c2000_config.h"

#if CONFIG_ENABLE_CLI_LFS_COMMAND

#if CONFIG_ENABLE_CLI_LFS_CMD_FORMAT
static Int16 FuncLfsFormat(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    memset(write_buffer, 0, bufferLen);

    if(lfs_c2000_format() == LFS_ERR_OK) {
        System_snprintf(write_buffer, bufferLen,
                "    OK\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
    }
    return 0;
}

static const CLI_Command_Definition_t lfs_cmd_format = {
    "lfs_format",
    "lfs_format:\r\n"
    "    Formats the storage device\r\n\r\n",
    FuncLfsFormat,
    0
};
#endif /* CONFIG_ENABLE_CLI_LFS_CMD_FORMAT */

void CMD_LFS_init(void)
{
#if CONFIG_ENABLE_CLI_LFS_CMD_FORMAT
    CLIRegisterCommand(&lfs_cmd_format);
#endif /* CONFIG_ENABLE_CLI_LFS_CMD_FORMAT */
}

#endif /* CONFIG_ENABLE_CLI_LFS_COMMAND */

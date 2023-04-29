/*!
 * \file cmd_lfs.c
 */

#include "autoconf.h"
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include "FreeRTOS_CLI.h"
#include "../littlefs/lfs.h"
#include "../lfs_c2000_config.h"
#include "fram/fm25w256/fm25w256.h"

#if CONFIG_ENABLE_CLI_LFS_COMMAND

#pragma DATA_SECTION(lfsCmdBuffer, "DMARAM")
FM25W256_DECLARE_BUFFER(lfsCmdBuffer, CONFIG_LFS_BLOCK_SZ);


static Int16 FuncLfsDump(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    static UInt16 internalState = 0;
    static UInt16 nBytes = 0;
    static UInt16 printIdx = 0;

    char * ptrStrParam;
    Int16 strParamLen;
    char *ptrEnd;
    char tmpStr[12];
    Int32 i32Temp;
    UInt16 address;
    FM25W256_RET_T ret;

    memset(write_buffer, 0, bufferLen);

    if (internalState == 0) {
        /* Get Block address */
        ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
        if (ptrStrParam == NULL) {
            System_snprintf(write_buffer, bufferLen, "    Error: Parameter1 not found!\r\n\r\n");
            return 0;
        }
        memcpy(tmpStr, ptrStrParam, strParamLen);
        tmpStr[strParamLen] = '\0';
        errno = 0;
        i32Temp = strtol(tmpStr, &ptrEnd, 0);
        if((ptrEnd == tmpStr) || (*ptrEnd != '\0') || (i32Temp < 0) ||
           (((i32Temp == LONG_MIN) || (i32Temp == LONG_MAX)) && (errno == ERANGE)) ||
           ((i32Temp * CONFIG_LFS_BLOCK_SZ) >= CONFIG_SPI_FM25W256_SIZE)) {
            /* parameter is not a number */
            System_snprintf(write_buffer, bufferLen,
                    "    Error: Parameter 1 value is invalid.\r\n\r\n");
            return 0;
        }
        address = (UInt16)(i32Temp & 0xFFFF) * CONFIG_LFS_BLOCK_SZ;
        memset(lfsCmdBuffer, 0xFF, sizeof(lfsCmdBuffer));
        FM25W256_SET_ADRESS(lfsCmdBuffer, address);
        ret = FM25W256_read(lfsCmdBuffer, CONFIG_LFS_BLOCK_SZ + FM25W256_OPCODE_SZ + FM25W256_ADDR_SZ);

        if(FM25W256_OK != ret) {
            System_snprintf(write_buffer, bufferLen,
                    "    Failed\r\n\r\n");
            return 0;
        }

        internalState++;
        printIdx = 0;
        nBytes = CONFIG_LFS_BLOCK_SZ;
        return 1;
    } else if (internalState == 1) {
        /* Display Values */
        System_snprintf(write_buffer, bufferLen, "    ");
        while(nBytes > 0) {
            System_snprintf(tmpStr, 8, "%02x ", (int)FM25W256_BUFFER(lfsCmdBuffer, printIdx));
            strcat(write_buffer, tmpStr);
            nBytes--;
            printIdx++;
            if((printIdx % 16) == 0) {
                strcat(write_buffer, "\r\n");
                return 1;
            }
        }
        /* Done printing */
        strcat(write_buffer, "\r\n\r\n");
        internalState = 0;
        printIdx = 0;
    } else {
        internalState = 0;
    }
    return 0;
}

static const CLI_Command_Definition_t lfs_cmd_dump = {
    "lfs_dump",
    "lfs_dump <block id>:\r\n"
    "    Dump the <block id>\r\n\r\n",
    FuncLfsDump,
    1
};


static Int16 FuncLfsErase(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    static UInt16 internalState = 0;
    static UInt16 blockToDelete = 0;
    static int32_t remaining = 0;

    char * ptrStrParam;
    Int16 strParamLen;
    char *ptrEnd;
    char tmpStr[12];
    Int32 i32Temp;
    UInt16 address;
    FM25W256_RET_T ret;

    memset(write_buffer, 0, bufferLen);

    if(internalState == 0) {
        /* Get Block address */
        ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
        if (ptrStrParam == NULL) {
            System_snprintf(write_buffer, bufferLen, "    Error: Parameter1 not found!\r\n\r\n");
            return 0;
        }
        memcpy(tmpStr, ptrStrParam, strParamLen);
        tmpStr[strParamLen] = '\0';
        errno = 0;
        i32Temp = strtol(tmpStr, &ptrEnd, 0);
        if((ptrEnd == tmpStr) || (*ptrEnd != '\0') ||
           (((i32Temp == LONG_MIN) || (i32Temp == LONG_MAX)) && (errno == ERANGE))) {
            /* parameter is not a number */
            System_snprintf(write_buffer, bufferLen,
                    "    Error: Parameter 1 value is invalid.\r\n\r\n");
            return 0;
        }

        if((i32Temp < -1) ||
           (i32Temp >= (CONFIG_SPI_FM25W256_SIZE / CONFIG_LFS_BLOCK_SZ))){
            System_snprintf(write_buffer, bufferLen,
                    "    Invalid block id\r\n\r\n");
            return 0;
        } else if(i32Temp == -1) {
            /* Delete all */
            remaining = CONFIG_SPI_FM25W256_SIZE / CONFIG_LFS_BLOCK_SZ;
            blockToDelete = 0;
            internalState++;
        } else {
            /* Delete specified block */
            remaining = 1;
            blockToDelete = i32Temp;
            internalState++;
        }
        return 1;
    } else if(internalState == 1) {
        address = (UInt16)(blockToDelete & 0xFFFF) * CONFIG_LFS_BLOCK_SZ;
        FM25W256_SET_ADRESS(lfsCmdBuffer, address);
        memset(&(FM25W256_BUFFER(lfsCmdBuffer, 0)), 0xFF, CONFIG_LFS_BLOCK_SZ);
        ret = FM25W256_write(lfsCmdBuffer, CONFIG_LFS_BLOCK_SZ + FM25W256_OPCODE_SZ + FM25W256_ADDR_SZ);
        if(FM25W256_OK == ret) {
            System_snprintf(write_buffer, bufferLen,
                    "    Deleted block %d\r\n", blockToDelete);
            blockToDelete++;
            remaining--;
            if(remaining <= 0) {
                internalState++;
            }
        } else {
            System_snprintf(write_buffer, bufferLen,
                    "    Failed on block %d\r\n\r\n", blockToDelete);
            internalState++;
        }
        return 1;
    } else {
        internalState = 0;
        System_snprintf(write_buffer, bufferLen, "\r\n");
    }

    return 0;
}

static const CLI_Command_Definition_t lfs_cmd_erase = {
    "lfs_erase",
    "lfs_erase <block id>:\r\n"
    "    Erases the <block id> and initialize to 0xFF.\r\n"
    "    block_id value of -1 erases the entire storage.\r\n\r\n",
    FuncLfsErase,
    1
};


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


#if CONFIG_ENABLE_CLI_LFS_CMD_MOUNT

static Int16 FuncLfsMount(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    memset(write_buffer, 0, bufferLen);

    if(lfs_c2000_mount() != NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    OK\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
    }
    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_mount = {
    "lfs_mount",
    "lfs_mount:\r\n"
    "    Mounts the storage device\r\n\r\n",
    FuncLfsMount,
    0
};


static Int16 FuncLfsUmount(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    memset(write_buffer, 0, bufferLen);

    if(lfs_c2000_umount() == LFS_ERR_OK) {
        System_snprintf(write_buffer, bufferLen,
                "    OK\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
    }
    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_umount = {
    "lfs_umount",
    "lfs_umount:\r\n"
    "    Unmount the storage device\r\n\r\n",
    FuncLfsUmount,
    0
};

#endif /* CONFIG_ENABLE_CLI_LFS_CMD_MOUNT */


#if CONFIG_ENABLE_CLI_LFS_CMD_LS

static Int16 FuncLfsLs(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    char * ptrStrParam;
    Int16 strParamLen;

    memset(write_buffer, 0, bufferLen);

    /* Get directory name */
    ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
    if(ptrStrParam == NULL) {
        if(lfs_c2000_ls("/", write_buffer, bufferLen) != LFS_ERR_OK) {
                    System_snprintf(write_buffer, bufferLen,
                            "    Failed!\r\n\r\n");
        }
    } else {
        if(lfs_c2000_ls(ptrStrParam, write_buffer, bufferLen) != LFS_ERR_OK) {
            System_snprintf(write_buffer, bufferLen,
                    "    Failed!\r\n\r\n");
        }
    }
    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_ls = {
    "lfs_ls",
    "lfs_ls <directory>:\r\n"
    "    List the contents of directory\r\n\r\n",
    FuncLfsLs,
    -1
};

#endif /* CONFIG_ENABLE_CLI_LFS_CMD_LS */


#if CONFIG_ENABLE_CLI_LFS_CMD_MKDIR

static Int16 FuncLfsMkdir(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    char * ptrStrParam;
    Int16 strParamLen;

    memset(write_buffer, 0, bufferLen);

    /* Get directory name */
    ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
    if(ptrStrParam == NULL) {
        System_snprintf(write_buffer, bufferLen, "    Error: Parameter1 not found!\r\n\r\n");
        return 0;
    }

    if(lfs_c2000_mkdir(ptrStrParam) == LFS_ERR_OK) {
        System_snprintf(write_buffer, bufferLen,
                "    OK\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
    }

    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_mkdir = {
    "lfs_mkdir",
    "lfs_mkdir <directory>:\r\n"
    "    Creates a directory\r\n\r\n",
    FuncLfsMkdir,
    1
};

#endif /* CONFIG_ENABLE_CLI_LFS_CMD_MKDIR */


#if CONFIG_ENABLE_CLI_LFS_CMD_FOPEN

static Int16 FuncLfsFopen(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    char * ptrStrParam;
    Int16 strParamLen;

    memset(write_buffer, 0, bufferLen);

    /* Get file name */
    ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
    if(ptrStrParam == NULL) {
        System_snprintf(write_buffer, bufferLen, "    Error: Parameter1 not found!\r\n\r\n");
        return 0;
    }

    if(lfs_c2000_fopen(ptrStrParam) != NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    OK\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
    }

    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_fopen = {
    "lfs_fopen",
    "lfs_fopen <filename>:\r\n"
    "    Open file in RDWR mode\r\n\r\n",
    FuncLfsFopen,
    1
};


static Int16 FuncLfsFclose(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    memset(write_buffer, 0, bufferLen);

    if(lfs_c2000_fclose() == LFS_ERR_OK) {
        System_snprintf(write_buffer, bufferLen,
                "    OK\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
    }

    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_fclose = {
    "lfs_fclose",
    "lfs_fclose:\r\n"
    "    Closes already opened file\r\n\r\n",
    FuncLfsFclose,
    0
};


static Int16 FuncLfsWrite(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    char * ptrStrParam;
    Int16 strParamLen;

    memset(write_buffer, 0, bufferLen);

    ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
    if(ptrStrParam == NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
        return 0;
    }

    if(lfs_c2000_fwrite(ptrStrParam, strlen(ptrStrParam)) == strlen(ptrStrParam)) {
        System_snprintf(write_buffer, bufferLen,
                "    OK\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
    }

    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_fwrite = {
    "lfs_fwrite",
    "lfs_fwrite <data>:\r\n"
    "    Appends <data> to already opened file\r\n\r\n",
    FuncLfsWrite,
    -1
};


static Int16 FuncLfsRead(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    int ret = 0;

    memset(write_buffer, 0, bufferLen);

    ret = lfs_c2000_fread(write_buffer, bufferLen);

    if(ret == 0) {
        System_snprintf(write_buffer, bufferLen,
                "    Empty File\r\n\r\n");
    } else if(ret < 0) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed!\r\n\r\n");
    } else {
        strncat(write_buffer, "\r\n\r\n", bufferLen);
    }

    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_fread = {
    "lfs_fread",
    "lfs_fread:\r\n"
    "    Reads from already opened file\r\n\r\n",
    FuncLfsRead,
    0
};


static Int16 FuncLfsMv(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    char * ptrStrParamSrc;
    Int16 strParamSrcLen;
    char src[128];
    char * ptrStrParamTarget;
    Int16 strParamTargetLen;
    char target[128];

    memset(write_buffer, 0, bufferLen);

    /* Get source name */
    ptrStrParamSrc = (char *) CLIGetParameter(commandStr, 1, &strParamSrcLen);
    if((ptrStrParamSrc == NULL) || (strParamSrcLen > (sizeof(src) - 1))){
        System_snprintf(write_buffer, bufferLen, "    Error: Parameter1 not found!\r\n\r\n");
        return 0;
    }
    strncpy(src, ptrStrParamSrc, strParamSrcLen);
    src[strParamSrcLen] = 0;

    /* Get target name */
    ptrStrParamTarget = (char *) CLIGetParameter(commandStr, 2, &strParamTargetLen);
    if(ptrStrParamTarget == NULL) {
        System_snprintf(write_buffer, bufferLen, "    Error: Parameter1 not found!\r\n\r\n");
        return 0;
    }
    strncpy(target, ptrStrParamTarget, strParamTargetLen);
    target[strParamTargetLen] = 0;

    if(lfs_c2000_mv(src, target) == LFS_ERR_OK) {
        System_snprintf(write_buffer, bufferLen, "    OK.\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen, "    Failed!\r\n\r\n");
    }
    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_mv = {
    "lfs_mv",
    "lfs_mv <src> <taget>:\r\n"
    "    Moves/renames <src> to <target>\r\n\r\n",
    FuncLfsMv,
    2
};


static Int16 FuncLfsRm(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    char * ptrStrParamSrc;
    Int16 strParamSrcLen;
    char path[128];

    memset(write_buffer, 0, bufferLen);

    /* Get Full path name */
    ptrStrParamSrc = (char *) CLIGetParameter(commandStr, 1, &strParamSrcLen);
    if((ptrStrParamSrc == NULL) || (strParamSrcLen > (sizeof(path) - 1))){
        System_snprintf(write_buffer, bufferLen, "    Error: Parameter1 not found!\r\n\r\n");
        return 0;
    }
    strncpy(path, ptrStrParamSrc, strParamSrcLen);
    path[strParamSrcLen] = 0;

    if(lfs_c2000_rm(path) == LFS_ERR_OK) {
        System_snprintf(write_buffer, bufferLen, "    OK.\r\n\r\n");
    } else {
        System_snprintf(write_buffer, bufferLen, "    Failed!\r\n\r\n");
    }
    return 0;
}


static const CLI_Command_Definition_t lfs_cmd_rm = {
    "lfs_rm",
    "lfs_rm <path>:\r\n"
    "    Removes file or directory specified by <path>\r\n\r\n",
    FuncLfsRm,
    1
};


#endif /* CONFIG_ENABLE_CLI_LFS_CMD_FOPEN */


void CMD_LFS_init(void)
{
    CLIRegisterCommand(&lfs_cmd_dump);
    CLIRegisterCommand(&lfs_cmd_erase);

#if CONFIG_ENABLE_CLI_LFS_CMD_FORMAT
    CLIRegisterCommand(&lfs_cmd_format);
#endif /* CONFIG_ENABLE_CLI_LFS_CMD_FORMAT */

#if CONFIG_ENABLE_CLI_LFS_CMD_MOUNT
    CLIRegisterCommand(&lfs_cmd_mount);
    CLIRegisterCommand(&lfs_cmd_umount);
#endif /* CONFIG_ENABLE_CLI_LFS_CMD_MOUNT */

#if CONFIG_ENABLE_CLI_LFS_CMD_LS
    CLIRegisterCommand(&lfs_cmd_ls);
#endif /* CONFIG_ENABLE_CLI_LFS_CMD_LS */

#if CONFIG_ENABLE_CLI_LFS_CMD_MKDIR
    CLIRegisterCommand(&lfs_cmd_mkdir);
#endif /* CONFIG_ENABLE_CLI_LFS_CMD_MKDIR */

#if CONFIG_ENABLE_CLI_LFS_CMD_FOPEN
    CLIRegisterCommand(&lfs_cmd_fopen);
    CLIRegisterCommand(&lfs_cmd_fclose);
    CLIRegisterCommand(&lfs_cmd_fwrite);
    CLIRegisterCommand(&lfs_cmd_fread);
    CLIRegisterCommand(&lfs_cmd_mv);
    CLIRegisterCommand(&lfs_cmd_rm);
#endif /* CONFIG_ENABLE_CLI_LFS_CMD_FOPEN */
}

#endif /* CONFIG_ENABLE_CLI_LFS_COMMAND */

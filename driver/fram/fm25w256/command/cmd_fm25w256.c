#include "autoconf.h"
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include "FreeRTOS_CLI.h"
#include "../fm25w256.h"


#if CONFIG_ENABLE_CLI_FM25W256_COMMAND

#define FM25W256_CMD_BUFSZ      (64)
#pragma DATA_SECTION(fm25w256Buffer, "DMARAM")
FM25W256_DECLARE_BUFFER(fm25w256Buffer, FM25W256_CMD_BUFSZ);

#if CONFIG_ENABLE_CLI_FM25W256_CMD_WRITE

static Int16 FuncFM25W256Write(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    FM25W256_RET_T ret = FM25W256_OK;
    char * ptrStrParam;
    Int16 strParamLen;
    char *ptrEnd;
    UInt16 address;
    Int32 i32Temp;
    static Uint16 nBytes = 0;
    char tmpStr[12];

    memset(write_buffer, 0, bufferLen);

    /* Get address */
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
       (((i32Temp == LONG_MIN) || (i32Temp == LONG_MAX)) && (errno == ERANGE)) ||
       (i32Temp >= CONFIG_SPI_FM25W256_SIZE)) {
        /* parameter is not a number */
        System_snprintf(write_buffer, bufferLen,
                "    Error: Parameter 1 value is invalid.\r\n\r\n");
        return 0;
    }
    address = (UInt16)(i32Temp);
    FM25W256_SET_ADRESS(fm25w256Buffer, address);

    for(nBytes = 0; nBytes < FM25W256_CMD_BUFSZ; nBytes++) {
        /* Get data */
        ptrStrParam = (char *) CLIGetParameter(commandStr, 2 + nBytes, &strParamLen);
        if(ptrStrParam == NULL) {
            break;
        }
        memcpy(tmpStr, ptrStrParam, strParamLen);
        tmpStr[strParamLen] = '\0';
        errno = 0;
        i32Temp = strtol(tmpStr, &ptrEnd, 0);
        if((ptrEnd == tmpStr) || (*ptrEnd != '\0') ||
           (((i32Temp == LONG_MIN) || (i32Temp == LONG_MAX)) && (errno == ERANGE))) {
            break;
        }
        FM25W256_BUFFER(fm25w256Buffer, nBytes) = (UInt8)i32Temp;
    }

    if((nBytes != 0) && (nBytes <= FM25W256_CMD_BUFSZ)) {
        ret = FM25W256_write(fm25w256Buffer, nBytes + FM25W256_OPCODE_SZ + FM25W256_ADDR_SZ);
        if(FM25W256_OK != ret) {
            System_snprintf(write_buffer, bufferLen,
                    "    Error: FM25W256_write (%d).\r\n\r\n", ret);
            return 0;
        }
    } else {
        System_snprintf(write_buffer, bufferLen,
                "    Warning: Invalid data count.\r\n\r\n");
        return 0;
    }

    System_snprintf(write_buffer, bufferLen,
            "    OK.\r\n\r\n");
    return 0;
}

static const CLI_Command_Definition_t fm25w256_write = {
    "fm25w256_write",
    "fm25w256_write <addr> <data>:\r\n"
    "    Writes <data> into FM25W256 in <addr>\r\n\r\n",
    FuncFM25W256Write,
    -1
};

#endif /* CONFIG_ENABLE_CLI_FM25W256_CMD_WRITE */

#if CONFIG_ENABLE_CLI_FM25W256_CMD_READ

static Int16 FuncFM25W256Read(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    static UInt16 internalState = 0;
    static UInt16 nBytes = 0;
    static UInt16 printIdx = 0;
    static UInt16 address = 0;
    Int32 i32Temp;
    char tmpStr[12];
    FM25W256_RET_T retval = FM25W256_OK;

    char * ptrStrParam;
    Int16 strParamLen;
    char *ptrEnd;

    memset(write_buffer, 0, bufferLen);

    if (internalState == 0) {
        /* Get parameter 1 (address) */
        ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
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
        address = (UInt16)i32Temp;

        /* Get parameter 2 (count) */
        ptrStrParam = (char *) CLIGetParameter(commandStr, 2, &strParamLen);
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
        nBytes = (UInt16)i32Temp;

        if(nBytes > FM25W256_CMD_BUFSZ) {
            System_snprintf(write_buffer, bufferLen,
                    "    Error: count too large.\r\n\r\n");
            return 0;
        } else if(nBytes <= 0) {
            System_snprintf(write_buffer, bufferLen,
                    "    Error: count is zero.\r\n\r\n");
            return 0;
        }

        memset(fm25w256Buffer, 0xFF, sizeof(fm25w256Buffer));
        FM25W256_SET_ADRESS(fm25w256Buffer, address);
        retval = FM25W256_read(fm25w256Buffer, nBytes + FM25W256_OPCODE_SZ + FM25W256_ADDR_SZ);
        if(FM25W256_OK != retval) {
            System_snprintf(write_buffer, bufferLen,
                    "    Error: FM25W256_read (%d).\r\n\r\n", retval);
            return 0;
        }

        internalState++;
        printIdx = 0;
        return 1;
    } else if (internalState == 1) {
        /* Display FRAM values */
        System_snprintf(write_buffer, bufferLen, "    ");
        while(nBytes > 0) {
            System_snprintf(tmpStr, 8, "%02x ", (int)FM25W256_BUFFER(fm25w256Buffer, printIdx));
            strcat(write_buffer, tmpStr);
            nBytes--;
            printIdx++;
            if((printIdx % 8) == 0) {
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

static const CLI_Command_Definition_t fm25w256_read = {
    "fm25w256_read",
    "fm25w256_read <addr> <count>:\r\n"
    "    Reads <count> data from FM25W256 address <addr>\r\n\r\n",
    FuncFM25W256Read,
    2
};

#endif /* CONFIG_ENABLE_CLI_FM25W256_CMD_READ */

void CMD_FM25W256_init(void)
{
#if CONFIG_ENABLE_CLI_FM25W256_CMD_WRITE
    CLIRegisterCommand(&fm25w256_write);
#endif /* CONFIG_ENABLE_CLI_FM25W256_CMD_WRITE */

#if CONFIG_ENABLE_CLI_FM25W256_CMD_READ
    CLIRegisterCommand(&fm25w256_read);
#endif /* CONFIG_ENABLE_CLI_FM25W256_CMD_READ */
}

#endif /* CONFIG_ENABLE_CLI_FM25W256_COMMAND */

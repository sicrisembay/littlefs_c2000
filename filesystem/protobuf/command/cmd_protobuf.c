/*!
 * \file cmd_protobuf.c
 */

#include "autoconf.h"
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include "FreeRTOS_CLI.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "../test.pb.h"
#include "lfs.h"

#if CONFIG_PROTOBUF_TEST

static Int16 FuncPbUpdate(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    pb_byte_t buffer[128] = {0};
    Int32 i32Temp;
    char tmpStr[12];
    char * ptrStrParam;
    Int16 strParamLen;
    char *ptrEnd;

    memset(write_buffer, 0, bufferLen);

    /* Get value */
    ptrStrParam = (char *) CLIGetParameter(commandStr, 1, &strParamLen);
    if(ptrStrParam == NULL) {
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

    if(lfs_c2000_mount() == NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to mount LFS\r\n\r\n");
        return 0;
    }

    lfs_c2000_rm("pbTest.bin");

    if(lfs_c2000_fopen("pbTest.bin") == NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to open pbTest.bin\r\n\r\n");
        return 0;
    }

    SimpleTest message = SimpleTest_init_zero;
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    message.test_value = i32Temp;
    bool status = pb_encode(&ostream, SimpleTest_fields, &message);
    size_t message_length = ostream.bytes_written;

    if(lfs_c2000_fwrite((char *)buffer, message_length) != message_length) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to write to pbTest.bin\r\n\r\n");
        lfs_c2000_fclose();
        lfs_c2000_umount();
        return 0;
    }

    lfs_c2000_fclose();
    lfs_c2000_umount();
    System_snprintf(write_buffer, bufferLen,
            "    OK\r\n\r\n");

    return 0;
}

static const CLI_Command_Definition_t pb_cmd_update = {
    "pb_update",
    "pb_update <value>:\r\n"
    "    pb_update opens a file pbTest.bin, decode, update the value,\r\n"
    "    encode, and store it back to the pbTest.bin.\r\n\r\n",
    FuncPbUpdate,
    1
};


static Int16 FuncPbGet(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    pb_byte_t buffer[128] = {0};
    size_t message_length;
    bool status;

    memset(write_buffer, 0, bufferLen);

    if(lfs_c2000_mount() == NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to mount LFS\r\n\r\n");
        return 0;
    }

    if(lfs_c2000_fopen("pbTest.bin") == NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to open pbTest.bin\r\n\r\n");
        lfs_c2000_umount();
        return 0;
    }

    message_length = lfs_c2000_fread((char*)buffer, sizeof(buffer));

    if(message_length < 0) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to read pbTest.bin\r\n\r\n");
        lfs_c2000_fclose();
        lfs_c2000_umount();
        return 0;
    }

    SimpleTest message = SimpleTest_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, message_length);
    status = pb_decode(&istream, SimpleTest_fields, &message);

    if(!status) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to decode pbTest.bin\r\n\r\n");
        lfs_c2000_fclose();
        lfs_c2000_umount();
        return 0;
    }

    lfs_c2000_fclose();
    lfs_c2000_umount();

    System_snprintf(write_buffer, bufferLen,
            "\r\n    Message: test_value is %ld\r\n\r\n", message.test_value);

    return 0;
}


static const CLI_Command_Definition_t pb_cmd_get = {
    "pb_get",
    "pb_get:\r\n"
    "    pb_get prints the protobuf message stored in pbTest.bin\r\n\r\n",
    FuncPbGet,
    0
};


static Int16 FuncPbInit(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    pb_byte_t buffer[128];
    size_t message_length;
    bool status;

    memset(write_buffer, 0, bufferLen);

    if(lfs_c2000_mount() == NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to mount LFS\r\n\r\n");
        return 0;
    }

    lfs_c2000_rm("pbTest.bin");

    if(lfs_c2000_fopen("pbTest.bin") == NULL) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to create pbTest.bin\r\n\r\n");
        return 0;
    }

    SimpleTest message = SimpleTest_init_zero;
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    message.test_value = 1;
    status = pb_encode(&stream, SimpleTest_fields, &message);
    message_length = stream.bytes_written;

    if(lfs_c2000_fwrite(buffer, message_length) != message_length) {
        System_snprintf(write_buffer, bufferLen,
                "    Failed to write to pbTest.bin\r\n\r\n");
        lfs_c2000_fclose();
        lfs_c2000_umount();
        return 0;
    }

    lfs_c2000_fclose();
    lfs_c2000_umount();

    System_snprintf(write_buffer, bufferLen,
            "\r\n    OK\r\n\r\n");

    return 0;
}


static const CLI_Command_Definition_t pb_cmd_init = {
    "pb_init",
    "pb_init:\r\n"
    "    pb_init creates pbTest.bin with default values\r\n\r\n",
    FuncPbInit,
    0
};

void CMD_PBTEST_init(void)
{
    CLIRegisterCommand(&pb_cmd_update);
    CLIRegisterCommand(&pb_cmd_get);
    CLIRegisterCommand(&pb_cmd_init);
}

#endif /* CONFIG_PROTOBUF_TEST */

#include "driver_def.h"
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Assert.h>
#include "DSP2833x_Device.h"
#include "FreeRTOS_CLI.h"
#include "../spi.h"

#if (CONFIG_ENABLE_CLI_SPI_COMMAND && (CONFIG_ENABLE_CLI_SPI_CMD_TRANSACT || CONFIG_ENABLE_CLI_FIFO_SPI_CMD_TRANSACT))

#define CONCAT(x, y)        x##y
#define CONCAT_L1(x,y)      CONCAT(x,y)

#if defined(CONFIG_CLI_SPI_MCBSPA_CS_GPIO)
#if ((CONFIG_CLI_SPI_MCBSPA_CS_GPIO >= 0) && (CONFIG_CLI_SPI_MCBSPA_CS_GPIO <= 31))
#define CLI_SPI_CS_MCBSPA_SET      CONCAT_L1(GpioDataRegs.GPASET.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPA_CLEAR    CONCAT_L1(GpioDataRegs.GPACLEAR.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPA_PUD      CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPA_DIR      CONCAT_L1(GpioCtrlRegs.GPADIR.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 1
#if (CONFIG_CLI_SPI_MCBSPA_CS_GPIO <= 15)
#define CLI_SPI_CS_MCBSPA_MUX      CONCAT_L1(GpioCtrlRegs.GPAMUX1.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 0
#else
#define CLI_SPI_CS_MCBSPA_MUX      CONCAT_L1(GpioCtrlRegs.GPAMUX2.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 0
#endif
#elif ((CONFIG_CLI_SPI_MCBSPA_CS_GPIO >= 32) && (CONFIG_CLI_SPI_MCBSPA_CS_GPIO <= 63))
#define CLI_SPI_CS_MCBSPA_SET      CONCAT_L1(GpioDataRegs.GPBSET.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPA_CLEAR    CONCAT_L1(GpioDataRegs.GPBCLEAR.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPA_PUD      CONCAT_L1(GpioCtrlRegs.GPBPUD.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPA_DIR      CONCAT_L1(GpioCtrlRegs.GPBDIR.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 1
#if (CONFIG_CLI_SPI_MCBSPA_CS_GPIO <= 47)
#define CLI_SPI_CS_MCBSPA_MUX      CONCAT_L1(GpioCtrlRegs.GPBMUX1.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 0
#else
#define CLI_SPI_CS_MCBSPA_MUX      CONCAT_L1(GpioCtrlRegs.GPBMUX2.bit.GPIO, CONFIG_CLI_SPI_MCBSPA_CS_GPIO) = 0
#endif
#else
#error "Unsupported SPI MCBSP-A Test CS GPIO"
#endif
#endif /* defined(CONFIG_CLI_SPI_MCBSPA_CS_GPIO) */

#if defined(CONFIG_CLI_SPI_MCBSPB_CS_GPIO)
#if ((CONFIG_CLI_SPI_MCBSPB_CS_GPIO >= 0) && (CONFIG_CLI_SPI_MCBSPB_CS_GPIO <= 31))
#define CLI_SPI_CS_MCBSPB_SET      CONCAT_L1(GpioDataRegs.GPASET.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPB_CLEAR    CONCAT_L1(GpioDataRegs.GPACLEAR.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPB_PUD      CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPB_DIR      CONCAT_L1(GpioCtrlRegs.GPADIR.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 1
#if (CONFIG_CLI_SPI_MCBSPB_CS_GPIO <= 15)
#define CLI_SPI_CS_MCBSPB_MUX      CONCAT_L1(GpioCtrlRegs.GPAMUX1.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 0
#else
#define CLI_SPI_CS_MCBSPB_MUX      CONCAT_L1(GpioCtrlRegs.GPAMUX2.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 0
#endif
#elif ((CONFIG_CLI_SPI_MCBSPB_CS_GPIO >= 32) && (CONFIG_CLI_SPI_MCBSPB_CS_GPIO <= 63))
#define CLI_SPI_CS_MCBSPB_SET      CONCAT_L1(GpioDataRegs.GPBSET.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPB_CLEAR    CONCAT_L1(GpioDataRegs.GPBCLEAR.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPB_PUD      CONCAT_L1(GpioCtrlRegs.GPBPUD.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 1
#define CLI_SPI_CS_MCBSPB_DIR      CONCAT_L1(GpioCtrlRegs.GPBDIR.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 1
#if (CONFIG_CLI_SPI_MCBSPB_CS_GPIO <= 47)
#define CLI_SPI_CS_MCBSPB_MUX      CONCAT_L1(GpioCtrlRegs.GPBMUX1.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 0
#else
#define CLI_SPI_CS_MCBSPB_MUX      CONCAT_L1(GpioCtrlRegs.GPBMUX2.bit.GPIO, CONFIG_CLI_SPI_MCBSPB_CS_GPIO) = 0
#endif
#else
#error "Unsupported SPI MCBSP-B Test CS GPIO"
#endif
#endif /* defined(CONFIG_CLI_SPI_MCBSPB_CS_GPIO) */

#if defined(CONFIG_CLI_SPI_FIFO_CS_GPIO)
#if ((CONFIG_CLI_SPI_FIFO_CS_GPIO >= 0) && (CONFIG_CLI_SPI_FIFO_CS_GPIO <= 31))
#define CLI_SPI_CS_FIFO_SET      CONCAT_L1(GpioDataRegs.GPASET.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 1
#define CLI_SPI_CS_FIFO_CLEAR    CONCAT_L1(GpioDataRegs.GPACLEAR.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 1
#define CLI_SPI_CS_FIFO_PUD      CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 1
#define CLI_SPI_CS_FIFO_DIR      CONCAT_L1(GpioCtrlRegs.GPADIR.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 1
#if (CONFIG_CLI_SPI_FIFO_CS_GPIO <= 15)
#define CLI_SPI_CS_FIFO_MUX      CONCAT_L1(GpioCtrlRegs.GPAMUX1.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 0
#else
#define CLI_SPI_CS_FIFO_MUX      CONCAT_L1(GpioCtrlRegs.GPAMUX2.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 0
#endif
#elif ((CONFIG_CLI_SPI_FIFO_CS_GPIO >= 32) && (CONFIG_CLI_SPI_FIFO_CS_GPIO <= 63))
#define CLI_SPI_CS_FIFO_SET      CONCAT_L1(GpioDataRegs.GPBSET.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 1
#define CLI_SPI_CS_FIFO_CLEAR    CONCAT_L1(GpioDataRegs.GPBCLEAR.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 1
#define CLI_SPI_CS_FIFO_PUD      CONCAT_L1(GpioCtrlRegs.GPBPUD.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 1
#define CLI_SPI_CS_FIFO_DIR      CONCAT_L1(GpioCtrlRegs.GPBDIR.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 1
#if (CONFIG_CLI_SPI_FIFO_CS_GPIO <= 47)
#define CLI_SPI_CS_FIFO_MUX      CONCAT_L1(GpioCtrlRegs.GPBMUX1.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 0
#else
#define CLI_SPI_CS_FIFO_MUX      CONCAT_L1(GpioCtrlRegs.GPBMUX2.bit.GPIO, CONFIG_CLI_SPI_FIFO_CS_GPIO) = 0
#endif
#else
#error "Unsupported SPI FIFO Test CS GPIO"
#endif
#endif /* defined(CONFIG_CLI_SPI_FIFO_CS_GPIO) */

#define SPI_CMD_BUFSZ   (32)
#pragma DATA_SECTION(spiBuffer, "DMARAM")
static UInt8 spiBuffer[SPI_CMD_BUFSZ];
static Semaphore_Handle sem_sync = NULL;

#if defined(CONFIG_CLI_SPI_MCBSPA_CS_GPIO)
static void CLI_SPI_CS_MCBSPA_select(Bool bSelect)
{
    if(bSelect) {
        /* Select: Active Low */
        CLI_SPI_CS_MCBSPA_CLEAR;
    } else {
        /* Deselect */
        CLI_SPI_CS_MCBSPA_SET;
    }
}
#endif /* defined(CONFIG_CLI_SPI_MCBSPA_CS_GPIO) */

#if defined(CONFIG_CLI_SPI_MCBSPB_CS_GPIO)
static void CLI_SPI_CS_MCBSPB_select(Bool bSelect)
{
    if(bSelect) {
        /* Select: Active Low */
        CLI_SPI_CS_MCBSPB_CLEAR;
    } else {
        /* Deselect */
        CLI_SPI_CS_MCBSPB_SET;
    }
}
#endif /* defined(CONFIG_CLI_SPI_MCBSPB_CS_GPIO) */

#if defined(CONFIG_CLI_SPI_FIFO_CS_GPIO)
static void CLI_SPI_CS_FIFO_select(Bool bSelect)
{
    if(bSelect) {
        /* Select: Active Low */
        CLI_SPI_CS_FIFO_CLEAR;
    } else {
        /* Deselect */
        CLI_SPI_CS_FIFO_SET;
    }
}
#endif /* defined(CONFIG_CLI_SPI_FIFO_CS_GPIO) */

#if CONFIG_ENABLE_CLI_SPI_CMD_LIST

static Int16 FuncSpiList(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    char tmpStrVal[24];
    memset(write_buffer, 0, bufferLen);
    System_snprintf(write_buffer, bufferLen, "    %d bus found\r\n\r\n", N_SPI_BUS);
#if CONFIG_ENABLE_MCBSPA_SPI
    System_snprintf(tmpStrVal, sizeof(tmpStrVal), "    %d : MCBSP-A\r\n", SPI_BUS_MCBSP_A);
    strcat(write_buffer, tmpStrVal);
#endif
#if CONFIG_ENABLE_MCBSPB_SPI
    System_snprintf(tmpStrVal, sizeof(tmpStrVal), "    %d : MCBSP-B\r\n", SPI_BUS_MCBSP_B);
    strcat(write_buffer, tmpStrVal);
#endif
#if CONFIG_ENABLE_FIFO_SPI
    System_snprintf(tmpStrVal, sizeof(tmpStrVal), "    %d : SPI-FIFO\r\n", SPI_BUS_FIFO);
    strcat(write_buffer, tmpStrVal);
#endif
    strcat(write_buffer, "\r\n");

    return 0;
}


static const CLI_Command_Definition_t spi_list = {
    "spi_list",
    "spi_list:\r\n"
    "    List the available SPI bus\r\n\r\n",
    FuncSpiList,
    0
};

#endif /* CONFIG_ENABLE_CLI_SPI_CMD_LIST */


#if CONFIG_ENABLE_CLI_SPI_CMD_TRANSACT

static Int16 FuncSpiTransact(char * write_buffer, size_t bufferLen, const char *commandStr)
{
    static UInt16 internalState = 0;
    static UInt16 printIdx = 0;
    char * ptrStrParam;
    Int16 strParamLen;
    char *ptrEnd;
    Int32 i32Temp;
    static Uint16 nBytes = 0;
    char tmpStr[12];
    SPI_RET_T ret_spi = SPI_OK;
    SPI_BUS_T spi_id = N_SPI_BUS;
    SPI_MODE_T spi_mode = N_SPI_MODE;
    Bool bResult;
    char tmpStrVal[8];

    memset(write_buffer, 0, bufferLen);

    if(internalState == 0) {
        /* SPI bus ID */
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
        if(i32Temp >= N_SPI_BUS) {
            System_snprintf(write_buffer, bufferLen,
                    "    Error: Invalid SPI bus.\r\n\r\n");
            return 0;
        }
        spi_id = (SPI_BUS_T)i32Temp;
        /*
         * SPI bus MODE
         */
        ptrStrParam = (char *) CLIGetParameter(commandStr, 2, &strParamLen);
        if (ptrStrParam == NULL) {
            System_snprintf(write_buffer, bufferLen, "    Error: Parameter2 not found!\r\n\r\n");
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
                    "    Error: Parameter 2 value is invalid.\r\n\r\n");
            return 0;
        }
        if(i32Temp >= N_SPI_MODE) {
            System_snprintf(write_buffer, bufferLen,
                    "    Error: Invalid SPI mode.\r\n\r\n");
            return 0;
        }
        spi_mode = (SPI_MODE_T)i32Temp;
        /*
         * Data
         */
        for (nBytes = 0; nBytes < SPI_CMD_BUFSZ; nBytes++) {
            /* Get data */
            ptrStrParam = (char *) CLIGetParameter(commandStr, 3 + nBytes, &strParamLen);
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
            spiBuffer[nBytes] = (UInt8)i32Temp;
        }

        if((nBytes != 0) && (nBytes <= SPI_CMD_BUFSZ)) {
            switch(spi_id) {
#if defined(CONFIG_CLI_SPI_MCBSPA_CS_GPIO)
                case SPI_BUS_MCBSP_A: {
                    ret_spi = SPI_transact_8bit(spi_id, spiBuffer, nBytes, spi_mode, &bResult, sem_sync, CLI_SPI_CS_MCBSPA_select);
                    break;
                }
#endif
#if defined(CONFIG_CLI_SPI_MCBSPB_CS_GPIO)
                case SPI_BUS_MCBSP_B: {
                    ret_spi = SPI_transact_8bit(spi_id, spiBuffer, nBytes, spi_mode, &bResult, sem_sync, CLI_SPI_CS_MCBSPB_select);
                    break;
                }
#endif
#if defined(CONFIG_CLI_SPI_FIFO_CS_GPIO)
                case SPI_BUS_FIFO: {
                    ret_spi = SPI_transact_8bit(spi_id, spiBuffer, nBytes, spi_mode, &bResult, sem_sync, CLI_SPI_CS_FIFO_select);
                    break;
                }
#endif
                default: {
                    System_snprintf(write_buffer, bufferLen,
                            "    Error: Invalid SPI ID.\r\n\r\n");
                    return 0;
                }
            }
            if(SPI_OK != ret_spi) {
                System_snprintf(write_buffer, bufferLen,
                        "    Error: SPI_transact_8bit (%d).\r\n\r\n", ret_spi);
                return 0;
            }
            if(Semaphore_pend(sem_sync, 100)) {
                if(!bResult) {
                    System_snprintf(write_buffer, bufferLen,
                            "    Error: SPI_transact result error.\r\n\r\n");
                    return 0;
                }
            }
        } else {
            System_snprintf(write_buffer, bufferLen,
                    "    Warning: Invalid data count.\r\n\r\n");
            return 0;
        }

        internalState++;
        printIdx = 0;
        return 1;
    } else if (internalState == 1) {
        /* Display MISO Values */
        System_snprintf(write_buffer, bufferLen, "    ");
        while(nBytes > 0) {
            System_snprintf(tmpStrVal, 8, "%02x ", (int)spiBuffer[printIdx]);
            strcat(write_buffer, tmpStrVal);
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

static const CLI_Command_Definition_t spi_transact = {
    "spi_transact",
    "spi_transact <bus> <mode> <data>:\r\n"
    "    Send <data> to SPI <bus> MOSI and prints MISO data\r\n\r\n",
    FuncSpiTransact,
    -1
};
#endif /* CONFIG_ENABLE_CLI_SPI_CMD_TRANSACT */

void CMD_SPI_init(void)
{
    Error_Block eb;
    Semaphore_Params semParams;
    static UInt16 callOnce = 0;

    if(callOnce == 0) {
        callOnce = 1;

#if defined(CONFIG_CLI_SPI_MCBSPA_CS_GPIO)
        if(CONFIG_CLI_SPI_MCBSPA_CS_GPIO >= 0) {
            EALLOW;
            CLI_SPI_CS_MCBSPA_PUD;
            CLI_SPI_CS_MCBSPA_DIR;
            CLI_SPI_CS_MCBSPA_MUX;
            CLI_SPI_CS_MCBSPA_SET;
            EDIS;
        }
#endif /* defined(CONFIG_CLI_SPI_MCBSPA_CS_GPIO) */

#if defined(CONFIG_CLI_SPI_MCBSPB_CS_GPIO)
        if(CONFIG_CLI_SPI_MCBSPB_CS_GPIO >= 0) {
            EALLOW;
            CLI_SPI_CS_MCBSPB_PUD;
            CLI_SPI_CS_MCBSPB_DIR;
            CLI_SPI_CS_MCBSPB_MUX;
            CLI_SPI_CS_MCBSPB_SET;
            EDIS;
        }
#endif /* defined(CONFIG_CLI_SPI_MCBSPA_CS_GPIO) */

#if defined(CONFIG_CLI_SPI_FIFO_CS_GPIO)
        if(CONFIG_CLI_SPI_FIFO_CS_GPIO >= 0) {
            EALLOW;
            CLI_SPI_CS_FIFO_PUD;
            CLI_SPI_CS_FIFO_DIR;
            CLI_SPI_CS_FIFO_MUX;
            CLI_SPI_CS_FIFO_SET;
            EDIS;
        }
#endif /* defined(CONFIG_CLI_SPI_FIFO_CS_GPIO) */

        /*
         * Sync
         */
        Semaphore_Params_init(&semParams);
        semParams.instance->name = "cli_spi";
        semParams.mode = Semaphore_Mode_BINARY;
        sem_sync = Semaphore_create(0, &semParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (sem_sync != NULL), NULL);

#if CONFIG_ENABLE_CLI_SPI_CMD_LIST
        CLIRegisterCommand(&spi_list);
#endif /* CONFIG_ENABLE_CLI_SPI_CMD_LIST */

#if CONFIG_ENABLE_CLI_SPI_CMD_TRANSACT
        CLIRegisterCommand(&spi_transact);
#endif /* CONFIG_ENABLE_CLI_SPI_CMD_TRANSACT */

    }
}

#endif /* CONFIG_ENABLE_CLI_SPI_COMMAND */

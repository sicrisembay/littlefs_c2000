/*!
 * \file fm25w256.c
 */
#include "autoconf.h"
#include "command/cmd_fm25w256.h"
#include <stdbool.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Assert.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/gates/GateMutexPri.h>
#include "DSP2833x_Device.h"
#include "spi/spi.h"
#include "fram/fm25w256/fm25w256.h"

#if CONFIG_USE_SPI_FM25W256

#define CONCAT(x, y)        x##y
#define CONCAT_L1(x, y)     CONCAT(x, y)

#if ((CONFIG_SPI_FM25W256_CS >= 0) && (CONFIG_SPI_FM25W256_CS <= 31))
    #define FM25W256_CS_SET     CONCAT_L1(GpioDataRegs.GPASET.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_CLEAR   CONCAT_L1(GpioDataRegs.GPACLEAR.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_PUD     CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_DIR     CONCAT_L1(GpioCtrlRegs.GPADIR.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #if (CONFIG_SPI_FM25W256_CS <= 15)
        #define FM25W256_CS_MUX     CONCAT_L1(GpioCtrlRegs.GPAMUX1.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 0
    #else
        #define FM25W256_CS_MUX     CONCAT_L1(GpioCtrlRegs.GPAMUX2.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 0
    #endif
#elif ((CONFIG_SPI_FM25W256_CS >= 32) && (CONFIG_SPI_FM25W256_CS <= 63))
    #define FM25W256_CS_SET     CONCAT_L1(GpioDataRegs.GPBSET.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_CLEAR   CONCAT_L1(GpioDataRegs.GPBCLEAR.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_PUD     CONCAT_L1(GpioCtrlRegs.GPBPUD.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_DIR     CONCAT_L1(GpioCtrlRegs.GPBDIR.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #if (CONFIG_SPI_FM25W256_CS <= 47)
        #define FM25W256_CS_MUX     CONCAT_L1(GpioCtrlRegs.GPBMUX1.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 0
    #else
        #define FM25W256_CS_MUX     CONCAT_L1(GpioCtrlRegs.GPBMUX2.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 0
    #endif
#elif ((CONFIG_SPI_FM25W256_CS >= 64) && (CONFIG_SPI_FM25W256_CS <= 87))
    #define FM25W256_CS_SET     CONCAT_L1(GpioDataRegs.GPCSET.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_CLEAR   CONCAT_L1(GpioDataRegs.GPCCLEAR.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_PUD     CONCAT_L1(GpioCtrlRegs.GPCPUD.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #define FM25W256_CS_DIR     CONCAT_L1(GpioCtrlRegs.GPCDIR.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 1
    #if (CONFIG_SPI_FM25W256_CS <= 79)
        #define FM25W256_CS_MUX     CONCAT_L1(GpioCtrlRegs.GPCMUX1.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 0
    #else
        #define FM25W256_CS_MUX     CONCAT_L1(GpioCtrlRegs.GPCMUX2.bit.GPIO, CONFIG_SPI_FM25W256_CS) = 0
    #endif
#else
#error "Unsupported FM25W256 CS GPIO"
#endif


#if CONFIG_SPI_FM25W256_MCBSPA
    #define FM25W256_SPI_BUS    SPI_BUS_MCBSP_A
#elif CONFIG_SPI_FM25W256_MCBSPB
    #define FM25W256_SPI_BUS    SPI_BUS_MCBSP_B
#endif


#define FM25W256_WREN       (0x06)
#define FM25W256_WRDI       (0x04)
#define FM25W256_RDSR       (0x05)
#define FM25W256_WRSR       (0x01)
#define FM25W256_READ       (0x03)
#define FM25W256_WRITE      (0x02)


static Bool bInitDone = false;
static Semaphore_Handle sem_sync = NULL;
static GateMutexPri_Handle fm25w256_mutex = NULL;
static const char * tag = "dev_fm25w256";
#pragma DATA_SECTION(OpWrEn, "DMARAM")
static UInt8 OpWrEn;


static void FM25W256_CS_select(Bool bSelect)
{
    if(bSelect) {
        /* Select: Active Low */
        FM25W256_CS_CLEAR;
    } else {
        /* Deselect */
        FM25W256_CS_SET;
    }
}


static FM25W256_RET_T FM25W256_transact(Bool isWrite, UInt8 * pBuf, UInt16 len)
{
    SPI_RET_T ret_spi = SPI_OK;
    Bool bResult;
    IArg key;

    if(!bInitDone || !SPI_init_done()) {
        return FM25W256_INVALID_STATE;
    }

    if((pBuf == NULL) || (len == 0)) {
        return FM25W256_INVALID_ARG;
    }

    /* Take mutex */
    key = GateMutexPri_enter(fm25w256_mutex);

    if(isWrite) {
        /* WREN */
        OpWrEn = FM25W256_WREN;
        ret_spi = SPI_transact_8bit(FM25W256_SPI_BUS, &OpWrEn, 1, SPI_MODE0, &bResult, sem_sync, FM25W256_CS_select);
        if(SPI_OK != ret_spi) {
            GateMutexPri_leave(fm25w256_mutex, key);
            return FM25W256_ERROR;
        }
        if(Semaphore_pend(sem_sync, CONFIG_SPI_FM25W256_DEFAULT_TIMEOUT)) {
            if(!bResult) {
                GateMutexPri_leave(fm25w256_mutex, key);
                return FM25W256_ERROR;
            }
        } else {
            GateMutexPri_leave(fm25w256_mutex, key);
            return FM25W256_TIMEOUT;
        }
    }

    /* Burst Write or Read */
    if(isWrite) {
        pBuf[0] = FM25W256_WRITE;
    } else {
        pBuf[0] = FM25W256_READ;
    }
    ret_spi = SPI_transact_8bit(FM25W256_SPI_BUS, pBuf, len, SPI_MODE0, &bResult, sem_sync, FM25W256_CS_select);
    if(SPI_OK != ret_spi) {
        GateMutexPri_leave(fm25w256_mutex, key);
        return FM25W256_ERROR;
    }
    if(Semaphore_pend(sem_sync, CONFIG_SPI_FM25W256_DEFAULT_TIMEOUT)) {
        if(!bResult) {
            GateMutexPri_leave(fm25w256_mutex, key);
            return FM25W256_ERROR;
        }
    } else {
        GateMutexPri_leave(fm25w256_mutex, key);
        return FM25W256_TIMEOUT;
    }

    /* Release Mutex */
    GateMutexPri_leave(fm25w256_mutex, key);
    return FM25W256_OK;
}


FM25W256_RET_T FM25W256_init(void)
{
    Error_Block eb;
    Semaphore_Params semParams;
    GateMutexPri_Params mtxParams;

    /* Guard against multiple initialization */
    if(!bInitDone) {
        EALLOW;
        FM25W256_CS_PUD;
        FM25W256_CS_DIR;
        FM25W256_CS_MUX;
        FM25W256_CS_SET;
        EDIS;

        if(SPI_OK != SPI_init()) {
            return FM25W256_ERROR;
        }

        Error_init(&eb);

        /*
         * Mutex
         */
        GateMutexPri_Params_init(&mtxParams);
        mtxParams.instance->name = tag;
        fm25w256_mutex = GateMutexPri_create(&mtxParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (fm25w256_mutex != NULL), NULL);

        /*
         * Sync
         */
        Semaphore_Params_init(&semParams);
        semParams.instance->name = tag;
        semParams.mode = Semaphore_Mode_BINARY;
        sem_sync = Semaphore_create(0, &semParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (sem_sync != NULL), NULL);

        bInitDone = true;
    }

#if CONFIG_ENABLE_CLI_FM25W256_COMMAND
    CMD_FM25W256_init();
#endif /* CONFIG_ENABLE_CLI_FM25W256_COMMAND */

    System_printf("FM25W256 init done.\n\r");
    return FM25W256_OK;
}


FM25W256_RET_T FM25W256_write(UInt8 * pBuf, UInt16 len)
{
    return (FM25W256_transact(true, pBuf, len));
}


FM25W256_RET_T FM25W256_read(UInt8 * pBuf, UInt16 len)
{
    return (FM25W256_transact(false, pBuf, len));
}


#endif /* CONFIG_USE_SPI_FM25W256 */

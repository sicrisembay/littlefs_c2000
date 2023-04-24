#ifndef DRIVER_SPI_H
#define DRIVER_SPI_H

#include "autoconf.h"
#include "stdbool.h"
#include <ti/sysbios/knl/Semaphore.h>

#if (CONFIG_USE_SPI_DEVICE)

typedef void (* SPI_setChipSelect_cb)(Bool bSelect);

typedef enum {
    SPI_OK = 0,         /*!< No Error */
    SPI_INVALID_ARG,    /*!< Invalid argument */
    SPI_INVALID_STATE,  /*!< Not in a valid state */
    SPI_ERR_MBX_FULL,   /*!< Mailbox Full */
} SPI_RET_T;


typedef enum {
    SPI_MODE0 = 0,
    SPI_MODE1,
    SPI_MODE2,
    SPI_MODE3,
    N_SPI_MODE
} SPI_MODE_T;


typedef enum {
#if CONFIG_ENABLE_MCBSPA_SPI
    SPI_BUS_MCBSP_A,
#endif
#if CONFIG_ENABLE_MCBSPB_SPI
    SPI_BUS_MCBSP_B,
#endif
#if CONFIG_ENABLE_FIFO_SPI
    SPI_BUS_FIFO,
#endif
    N_SPI_BUS
} SPI_BUS_T;


SPI_RET_T SPI_init(void);
bool SPI_init_done(void);
SPI_RET_T SPI_transact_8bit(SPI_BUS_T id, UInt8 * pBuf, UInt16 len, SPI_MODE_T mode, Bool * pResult, Semaphore_Handle sem, SPI_setChipSelect_cb fcn_cs);
SPI_RET_T SPI_transact_16bit(SPI_BUS_T id, UInt16 * pBuf, UInt16 len, SPI_MODE_T mode, Bool * pResult, Semaphore_Handle sem, SPI_setChipSelect_cb fcn_cs);

#endif /* (CONFIG_USE_SPI_DEVICE) */
#endif /* DRIVER_SPI_H */

#include "autoconf.h"
#include "cli_command/cmd_spi.h"
#include <stdbool.h>
#include <xdc/std.h>
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Assert.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/family/c28/Hwi.h>
#include "spi.h"
#include "DSP2833x_Device.h"
#include "DSP2833x_Dma_defines.h"

#if (CONFIG_USE_SPI_DEVICE)

#define CONCAT(x, y)            x##y
#define CONCAT_L1(x, y)         CONCAT(x, y)

#if (CONFIG_ENABLE_MCBSPB_SPI)
#if (CONFIG_MCBSPB_SPI_MDX_GPIO <= 15)
#define MCBSPB_SPI_MOSI_MUX_CONFIG      CONCAT_L1(GpioCtrlRegs.GPAMUX1.bit.GPIO, CONFIG_MCBSPB_SPI_MDX_GPIO) = 3
#define MCBSPB_SPI_MOSI_SYNC_CONFIG     CONCAT_L1(GpioCtrlRegs.GPAQSEL1.bit.GPIO, CONFIG_MCBSPB_SPI_MDX_GPIO) = 3
#elif (CONFIG_MCBSPB_SPI_MDX_GPIO <= 31)
#define MCBSPB_SPI_MOSI_MUX_CONFIG      CONCAT_L1(GpioCtrlRegs.GPAMUX2.bit.GPIO, CONFIG_MCBSPB_SPI_MDX_GPIO) = 3
#define MCBSPB_SPI_MOSI_SYNC_CONFIG     CONCAT_L1(GpioCtrlRegs.GPAQSEL2.bit.GPIO, CONFIG_MCBSPB_SPI_MDX_GPIO) = 3
#else
#error "Unsupported GPIO for MOSI"
#endif
#if (CONFIG_MCBSPB_SPI_MDR_GPIO <= 15)
#define MCBSPB_SPI_MISO_MUX_CONFIG      CONCAT_L1(GpioCtrlRegs.GPAMUX1.bit.GPIO, CONFIG_MCBSPB_SPI_MDR_GPIO) = 3
#define MCBSPB_SPI_MISO_SYNC_CONFIG     CONCAT_L1(GpioCtrlRegs.GPAQSEL1.bit.GPIO, CONFIG_MCBSPB_SPI_MDR_GPIO) = 3
#elif (CONFIG_MCBSPB_SPI_MDR_GPIO <= 31)
#define MCBSPB_SPI_MISO_MUX_CONFIG      CONCAT_L1(GpioCtrlRegs.GPAMUX2.bit.GPIO, CONFIG_MCBSPB_SPI_MDR_GPIO) = 3
#define MCBSPB_SPI_MISO_SYNC_CONFIG     CONCAT_L1(GpioCtrlRegs.GPAQSEL2.bit.GPIO, CONFIG_MCBSPB_SPI_MDR_GPIO) = 3
#else
#error "Unsupported GPIO for MISO"
#endif
#if (CONFIG_MCBSPB_SPI_MCLKX_GPIO <= 15)
#define MCBSPB_SPI_MCLK_MUX_CONFIG      CONCAT_L1(GpioCtrlRegs.GPAMUX1.bit.GPIO, CONFIG_MCBSPB_SPI_MCLKX_GPIO) = 3
#elif (CONFIG_MCBSPB_SPI_MCLKX_GPIO <= 31)
#define MCBSPB_SPI_MCLK_MUX_CONFIG      CONCAT_L1(GpioCtrlRegs.GPAMUX2.bit.GPIO, CONFIG_MCBSPB_SPI_MCLKX_GPIO) = 3
#else
#error "Unsupported GPIO for MISO"
#endif
#if CONFIG_MCBSPB_SPI_MDX_PULLUP_ENABLE
#define MCBSPB_SPI_MOSI_PULLUP_CONFIG   CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_MCBSPB_SPI_MDX_GPIO) = 0
#else
#define MCBSPB_SPI_MOSI_PULLUP_CONFIG   CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_MCBSPB_SPI_MDX_GPIO) = 1
#endif /* CONFIG_MCBSPB_SPI_MDX_PULLUP_ENABLE */
#if CONFIG_MCBSPB_SPI_MDR_PULLUP_ENABLE
#define MCBSPB_SPI_MISO_PULLUP_CONFIG   CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_MCBSPB_SPI_MDR_GPIO) = 0
#else
#define MCBSPB_SPI_MISO_PULLUP_CONFIG   CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_MCBSPB_SPI_MDR_GPIO) = 1
#endif /* CONFIG_MCBSPB_SPI_MDR_PULLUP_ENABLE */
#if MCBSPB_SPI_MCLX_PULLUP_ENABLE
#define MCBSPB_SPI_MCLK_PULLUP_CONFIG   CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_MCBSPB_SPI_MCLKX_GPIO) = 0
#else
#define MCBSPB_SPI_MCLK_PULLUP_CONFIG   CONCAT_L1(GpioCtrlRegs.GPAPUD.bit.GPIO, CONFIG_MCBSPB_SPI_MCLKX_GPIO) = 1
#endif /* MCBSPB_SPI_MCLX_PULLUP_ENABLE */
#endif /* CONFIG_ENABLE_MCBSPB_SPI */

#if CONFIG_ENABLE_FIFO_SPI
#define SPI_CLKIN_FREQ_HZ   (((UInt32)CONFIG_SYSTEM_FREQ_MHZ * 1000000U) / 4U)
#define SPI_BAUDRATE_HZ     ((UInt32)CONFIG_FIFO_SPI_BAUDRATE_KHZ * 1000U)
#define FIFO_SPI_BRR_VALUE  ((SPI_CLKIN_FREQ_HZ / SPI_BAUDRATE_HZ) - 1)

#define FIFO_SPI_DATA_WIDTH_8       (0x7)
#define FIFO_SPI_DATA_WIDTH_16      (0xF)
#endif /* CONFIG_ENABLE_FIFO_SPI */

#define SPI_EVENT_TX_HWI        (Event_Id_00)
#define SPI_EVENT_RX_HWI        (Event_Id_01)
#define SPI_EVENT_ALL           (SPI_EVENT_TX_HWI | SPI_EVENT_RX_HWI)

typedef enum {
    SPI_DATA_WIDTH_8 = 0,
    SPI_DATA_WIDTH_16
} SPI_DATA_WIDTH_T;


typedef struct {
    UInt16 * pBuf;
    UInt16 len;
    SPI_DATA_WIDTH_T width;
    SPI_MODE_T mode;
    Semaphore_Handle semRequestor;
    SPI_setChipSelect_cb fcnCS_cb;
    Bool * pStatus;
} SPI_REQUEST_MB_OBJ_T;

typedef struct {
    Hwi_Handle hwiTx;
    Hwi_Handle hwiRx;
} SPI_HWI_HANDLE_T;

typedef enum {
    TX_CHANNEL = 0,
    RX_CHANNEL,
    N_SPI_CHANNEL
} SPI_CHANNEL_T;

static bool bInitDone = false;
static Task_Handle tskHdl_spi[N_SPI_BUS] = {NULL};
static Event_Handle spi_evt[N_SPI_BUS] = {NULL};
static Mailbox_Handle mbx_spi[N_SPI_BUS] = {NULL};
static SPI_HWI_HANDLE_T hwiHdl_spi[N_SPI_BUS];
static SPI_REQUEST_MB_OBJ_T spi_transaction_request[N_SPI_BUS];

static const char * tag[N_SPI_BUS] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    "mcbsp-a",
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    "mcbsp-b",
#endif
#if (CONFIG_ENABLE_FIFO_SPI)
    "fifo-spi",
#endif
};

static const char * tag_hwi[N_SPI_BUS][2] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    {
        "mcbsp-a-tx",
        "mcbsp-a-rx"
    },
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    {
        "mcbsp-b-tx",
        "mcbsp-b-rx"
    },
#endif
#if (CONFIG_ENABLE_FIFO_SPI)
    {
        "spi-fifo-tx",
        "spi-fifo-rx"
    },
#endif
};

#if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI)
volatile struct MCBSP_REGS * MCBSP_REGS_PTR[N_SPI_BUS] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    &McbspaRegs,
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    &McbspbRegs,
#endif
};
#endif /* (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI) */


static UInt16 const DEFAULT_SPI_TASK_STACK[N_SPI_BUS] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    CONFIG_TASK_STACK_MCBSPA_SPI,
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    CONFIG_TASK_STACK_MCBSPB_SPI,
#endif
#if (CONFIG_ENABLE_FIFO_SPI)
    CONFIG_TASK_STACK_FIFO_SPI,
#endif
};


static UInt16 const DEFAULT_SPI_TASK_PRIORITY[N_SPI_BUS] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    CONFIG_TASK_PRIO_MCBSPA_SPI,
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    CONFIG_TASK_PRIO_MCBSPB_SPI,
#endif
#if (CONFIG_ENABLE_FIFO_SPI)
    CONFIG_TASK_PRIO_FIFO_SPI,
#endif
};


static UInt16 const DEFAULT_SPI_MAX_TRANSACTION_REQUEST[N_SPI_BUS] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    CONFIG_MCBSPA_SPI_MAX_REQUEST_COUNT,
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    CONFIG_MCBSPB_SPI_MAX_REQUEST_COUNT,
#endif
#if (CONFIG_ENABLE_FIFO_SPI)
    CONFIG_FIFO_SPI_MAX_REQUEST_COUNT,
#endif
};

static UInt16 const DEFAULT_HWI_INT_NUM[N_SPI_BUS][N_SPI_CHANNEL] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    {
        80,     // McBsp-A Tx uses DMA Channel1
        81      // McBsp-A Rx uses DMA Channel2
    },
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    {
        82,     // McBsp-B Tx uses DMA Channel3
        83      // McBsp-B Rx uses DMA Channel4
    },
#endif
#if (CONFIG_ENABLE_FIFO_SPI)
    {
        73,     // FIFO Tx
        72      // FIFO Rx
    },
#endif
};

#if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI)
volatile struct CH_REGS * DMA_CH_REG_PTR[N_SPI_BUS][N_SPI_CHANNEL] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    {
        &(DmaRegs.CH1), // McBsp-A Tx uses DMA Channel1
        &(DmaRegs.CH2)  // McBsp-A Rx uses DMA Channel2
    },
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    {
        &(DmaRegs.CH3), // McBsp-B Tx uses DMA Channel3
        &(DmaRegs.CH4)  // McBsp-B Rx uses DMA Channel4
    },
#endif
};
#endif /* (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI) */

#if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI)
static UInt16 const DMA_EVT_SRC[N_SPI_BUS][N_SPI_CHANNEL] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    {
        DMA_MXEVTA, // McBsp-A Tx uses DMA Channel1
        DMA_MREVTA  // McBsp-A Rx uses DMA Channel2
    },
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    {
        DMA_MXREVTB, // McBsp-B Tx uses DMA Channel3
        DMA_MREVTB   // McBsp-B Rx uses DMA Channel4
    },
#endif
};
#endif /* (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI) */

static UInt32 const DEFAULT_TIMEOUT[N_SPI_BUS] = {
#if (CONFIG_ENABLE_MCBSPA_SPI)
    CONFIG_MCBSPA_SPI_DEFAULT_TIMEOUT,
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
    CONFIG_MCBSPB_SPI_DEFAULT_TIMEOUT,
#endif
#if (CONFIG_ENABLE_FIFO_SPI)
    CONFIG_FIFO_SPI_DEFAULT_TIMEOUT,
#endif
};


#if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI)
/*!
 * McBSP Interrupt Handler
 * This is called from SYS/BIOS interrupt dispatcher
 */
#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(spi_tx_dma_hwi_handler, "ramfuncs");
#endif
static Void spi_tx_dma_hwi_handler(UArg arg)
{
    SPI_BUS_T id = (SPI_BUS_T)arg;
    EALLOW;
    DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.HALT = 1;
    EDIS;

    if(spi_evt[id] != NULL) {
        Event_post(spi_evt[id], SPI_EVENT_TX_HWI);
    }
}


/*!
 * McBSP Interrupt Handler
 * This is called from SYS/BIOS interrupt dispatcher
 */
#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(spi_rx_dma_hwi_handler, "ramfuncs");
#endif
static Void spi_rx_dma_hwi_handler(UArg arg)
{
    SPI_BUS_T id = (SPI_BUS_T)arg;
    EALLOW;
    DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.HALT = 1;
    EDIS;

    /* De-Select Device */
    if(spi_transaction_request[id].fcnCS_cb != NULL) {
        spi_transaction_request[id].fcnCS_cb(FALSE);
    }

    if(spi_evt[id] != NULL) {
        Event_post(spi_evt[id], SPI_EVENT_RX_HWI);
    }
}


#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(tsk_spi_dma, "ramfuncs");
#endif
static void tsk_spi_dma(UArg a0, UArg a1)
{
    SPI_BUS_T id = (SPI_BUS_T)a0;
    UInt posted = 0;

    while(!bInitDone) {
        /* Wait till initialized */
        Task_sleep(1);
    }

#if (CONFIG_ENABLE_CLI_SPI_COMMAND && CONFIG_ENABLE_CLI_SPI_CMD_TRANSACT)
    CMD_SPI_init();
#endif

    while(1) {
        if(Mailbox_pend(mbx_spi[id], &spi_transaction_request[id], BIOS_WAIT_FOREVER)) {
            /* Check request validity */
            if((spi_transaction_request[id].len == 0) ||
               (spi_transaction_request[id].pBuf == NULL)) {
                /* Invalid request */
                if(spi_transaction_request[id].pStatus != NULL) {
                    *(spi_transaction_request[id].pStatus) = FALSE;
                }
                if(spi_transaction_request[id].semRequestor != NULL) {
                    Semaphore_post(spi_transaction_request[id].semRequestor);
                }
                continue;
            }

            /*
             * Update transaction Mode
             */
            switch(spi_transaction_request[id].mode) {
                case SPI_MODE0: {
                    MCBSP_REGS_PTR[id]->SPCR1.bit.CLKSTP = 3;  // Together with CLKXP/CLKRP determines clocking scheme
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKXP = 0;     // CPOL = 0, CPHA = 0
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKRP = 1;
                    break;
                }
                case SPI_MODE1: {
                    MCBSP_REGS_PTR[id]->SPCR1.bit.CLKSTP = 2;  // Together with CLKXP/CLKRP determines clocking scheme
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKXP = 0;     // CPOL = 0, CPHA = 1
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKRP = 0;
                    break;
                }
                case SPI_MODE2: {
                    MCBSP_REGS_PTR[id]->SPCR1.bit.CLKSTP = 3;  // Together with CLKXP/CLKRP determines clocking scheme
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKXP = 1;     // CPOL = 1, CPHA = 0
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKRP = 1;
                    break;
                }
                case SPI_MODE3: {
                    MCBSP_REGS_PTR[id]->SPCR1.bit.CLKSTP = 2;  // Together with CLKXP/CLKRP determines clocking scheme
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKXP = 1;     // CPOL = 1, CPHA = 1
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKRP = 0;
                    break;
                }
                default: {
                    /* Defaults to Mode 0 */
                    MCBSP_REGS_PTR[id]->SPCR1.bit.CLKSTP = 3;  // Together with CLKXP/CLKRP determines clocking scheme
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKXP = 0;     // CPOL = 0, CPHA = 0 rising edge no delay
                    MCBSP_REGS_PTR[id]->PCR.bit.CLKRP = 1;
                    break;
                }
            }
            /*
             * Update transaction data width
             */
            switch(spi_transaction_request[id].width) {
                case SPI_DATA_WIDTH_8: {
                    MCBSP_REGS_PTR[id]->RCR1.bit.RWDLEN1 = 0;  // 8-bit word
                    MCBSP_REGS_PTR[id]->XCR1.bit.XWDLEN1 = 0;  // 8-bit word
                    break;
                }
                case SPI_DATA_WIDTH_16: {
                    MCBSP_REGS_PTR[id]->RCR1.bit.RWDLEN1 = 2;  // 16-bit word
                    MCBSP_REGS_PTR[id]->XCR1.bit.XWDLEN1 = 2;  // 16-bit word
                    break;
                }
                default: {
                    MCBSP_REGS_PTR[id]->RCR1.bit.RWDLEN1 = 2;  // 16-bit word
                    MCBSP_REGS_PTR[id]->XCR1.bit.XWDLEN1 = 2;  // 16-bit word
                    break;
                }
            }

            /*
             * Initialize DMA for this transaction
             */
            EALLOW;
            /* McBSP transmit */
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.SOFTRESET = 1;
            DMA_CH_REG_PTR[id][TX_CHANNEL]->MODE.bit.CHINTE = 0;
            DMA_CH_REG_PTR[id][TX_CHANNEL]->BURST_SIZE.all = 0; // 1 word/burst
            DMA_CH_REG_PTR[id][TX_CHANNEL]->SRC_BURST_STEP = 0; // no effect when using 1 word/burst
            DMA_CH_REG_PTR[id][TX_CHANNEL]->DST_BURST_STEP = 0; // no effect when using 1 word/burst
            DMA_CH_REG_PTR[id][TX_CHANNEL]->TRANSFER_SIZE = spi_transaction_request[id].len - 1;
            DMA_CH_REG_PTR[id][TX_CHANNEL]->SRC_TRANSFER_STEP = 1; // Move to next word in buffer after each word in a burst
            DMA_CH_REG_PTR[id][TX_CHANNEL]->DST_TRANSFER_STEP = 0; // Don't move destination address
            DMA_CH_REG_PTR[id][TX_CHANNEL]->SRC_ADDR_SHADOW = (Uint32)(spi_transaction_request[id].pBuf);  // Source Address
            DMA_CH_REG_PTR[id][TX_CHANNEL]->SRC_BEG_ADDR_SHADOW = (Uint32)(spi_transaction_request[id].pBuf);
            DMA_CH_REG_PTR[id][TX_CHANNEL]->DST_ADDR_SHADOW = (Uint32)(&(MCBSP_REGS_PTR[id]->DXR1.all));  // Destination Address
            DMA_CH_REG_PTR[id][TX_CHANNEL]->DST_BEG_ADDR_SHADOW = (Uint32)(&(MCBSP_REGS_PTR[id]->DXR1.all));
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.PERINTCLR = 1;  // Clear peripheral interrupt event flag
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.SYNCCLR = 1;    // Clear sync flag
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.ERRCLR = 1;     // Clear sync error flag
            DMA_CH_REG_PTR[id][TX_CHANNEL]->DST_WRAP_SIZE = 0xFFFF;     // Max (don't want wrap)
            DMA_CH_REG_PTR[id][TX_CHANNEL]->SRC_WRAP_SIZE = 0xFFFF;     // Max (don't want wrap)
            DMA_CH_REG_PTR[id][TX_CHANNEL]->MODE.bit.SYNCE = 0;         // No sync signal
            DMA_CH_REG_PTR[id][TX_CHANNEL]->MODE.bit.SYNCSEL = 0;
            DMA_CH_REG_PTR[id][TX_CHANNEL]->MODE.bit.CHINTE = 1;        // Enable channel interrupt
            DMA_CH_REG_PTR[id][TX_CHANNEL]->MODE.bit.CHINTMODE = 1;     // Interrupt at end of transfer
            DMA_CH_REG_PTR[id][TX_CHANNEL]->MODE.bit.PERINTE = 1;       // Enable interrupt event
            DMA_CH_REG_PTR[id][TX_CHANNEL]->MODE.bit.PERINTSEL = DMA_EVT_SRC[id][TX_CHANNEL];
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.PERINTCLR = 1;  // Clear interrupt flag
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.SYNCCLR = 1;
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.ERRCLR = 1;
            /* McBSP receive */
            DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.SOFTRESET = 1;
            DMA_CH_REG_PTR[id][RX_CHANNEL]->MODE.bit.CHINTE = 0;
            DMA_CH_REG_PTR[id][RX_CHANNEL]->BURST_SIZE.all = 0;     // 1 word/burst
            DMA_CH_REG_PTR[id][RX_CHANNEL]->SRC_BURST_STEP = 0;     // no effect when using 1 word/burst
            DMA_CH_REG_PTR[id][RX_CHANNEL]->DST_BURST_STEP = 0;     // no effect when using 1 word/burst
            DMA_CH_REG_PTR[id][RX_CHANNEL]->TRANSFER_SIZE = spi_transaction_request[id].len - 1;
            DMA_CH_REG_PTR[id][RX_CHANNEL]->SRC_TRANSFER_STEP = 0;  // Don't move source address
            DMA_CH_REG_PTR[id][RX_CHANNEL]->DST_TRANSFER_STEP = 1;  // Move to next word in buffer after each word in a burst
            DMA_CH_REG_PTR[id][RX_CHANNEL]->SRC_ADDR_SHADOW = (Uint32)(&(MCBSP_REGS_PTR[id]->DRR1.all));  // Source Address
            DMA_CH_REG_PTR[id][RX_CHANNEL]->SRC_BEG_ADDR_SHADOW = (Uint32)(&(MCBSP_REGS_PTR[id]->DRR1.all));
            DMA_CH_REG_PTR[id][RX_CHANNEL]->DST_ADDR_SHADOW = (Uint32)(spi_transaction_request[id].pBuf);  // Destination Address
            DMA_CH_REG_PTR[id][RX_CHANNEL]->DST_BEG_ADDR_SHADOW = (Uint32)(spi_transaction_request[id].pBuf);
            DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.PERINTCLR = 1;  // Clear peripheral interrupt event flag
            DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.SYNCCLR = 1;    // Clear sync flag
            DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.ERRCLR = 1;     // Clear sync error flag
            DMA_CH_REG_PTR[id][RX_CHANNEL]->DST_WRAP_SIZE = 0xFFFF;     // Max (don't want wrap)
            DMA_CH_REG_PTR[id][RX_CHANNEL]->SRC_WRAP_SIZE = 0xFFFF;     // Max (don't want wrap)
            DMA_CH_REG_PTR[id][RX_CHANNEL]->MODE.bit.CHINTE = 1;        // Enable channel interrupt
            DMA_CH_REG_PTR[id][RX_CHANNEL]->MODE.bit.CHINTMODE = 1;     // Interrupt at end of transfer
            DMA_CH_REG_PTR[id][RX_CHANNEL]->MODE.bit.PERINTE = 1;       // Enable peripheral interrupt event
            DMA_CH_REG_PTR[id][RX_CHANNEL]->MODE.bit.PERINTSEL = DMA_EVT_SRC[id][RX_CHANNEL];
            DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.PERINTCLR = 1;  // Clear any spurious interrupt flags
            DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.SYNCCLR = 1;
            DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.ERRCLR = 1;
            EDIS;
            /*
             * Note: Clear previous event, if there's any.  From SysBios,
             * "pend returns immediately if the andMask OR orMask conditions are true upon entry."
             */
            while(Event_pend(spi_evt[id], 0, SPI_EVENT_TX_HWI | SPI_EVENT_RX_HWI, BIOS_NO_WAIT));

            /*
             * Select Device
             */
            if(spi_transaction_request[id].fcnCS_cb != NULL) {
                spi_transaction_request[id].fcnCS_cb(TRUE);
            }

            /*
             * Start DMA
             */
            EALLOW;
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.RUN = 1;
            DMA_CH_REG_PTR[id][RX_CHANNEL]->CONTROL.bit.RUN = 1;
            DMA_CH_REG_PTR[id][TX_CHANNEL]->CONTROL.bit.PERINTFRC = 1;  // Software force for a DMA burst transfer
            EDIS;

            /*
             * Wait for DMA transfer commplete event
             */
            posted = Event_pend(spi_evt[id], SPI_EVENT_TX_HWI | SPI_EVENT_RX_HWI, 0, DEFAULT_TIMEOUT[id]);

            if(posted == 0) {
                /*
                 * Error: operation timed out
                 */
                if(spi_transaction_request[id].pStatus != NULL) {
                    *(spi_transaction_request[id].pStatus) = FALSE;
                }
            } else {
                if(spi_transaction_request[id].pStatus != NULL) {
                    *(spi_transaction_request[id].pStatus) = TRUE;
                }
            }

            /*
             * Post to requester
             */
            if(spi_transaction_request[id].semRequestor != NULL) {
                Semaphore_post(spi_transaction_request[id].semRequestor);
            }
        }
    }
}
#endif /* (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI) */

#if CONFIG_ENABLE_FIFO_SPI
static void init_spi_fifo_gpio(void)
{
    EALLOW;

    SysCtrlRegs.PCLKCR0.bit.SPIAENCLK = 1;

    /*
     * Configure MOSI pin
     */
#if CONFIG_FIFO_SPI_MOSI_GPIO16
#if CONFIG_FIFO_SPI_MOSI_PULLUP_ENABLE
    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 0;
#else
    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 1;
#endif /* CONFIG_FIFO_SPI_MOSI_PULLUP_ENABLE */
    GpioCtrlRegs.GPAQSEL2.bit.GPIO16 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 1;
#elif CONFIG_FIFO_SPI_MOSI_GPIO54
#if CONFIG_FIFO_SPI_MOSI_PULLUP_ENABLE
    GpioCtrlRegs.GPBPUD.bit.GPIO54 = 0;
#else
    GpioCtrlRegs.GPBPUD.bit.GPIO54 = 1;
#endif /* CONFIG_FIFO_SPI_MOSI_PULLUP_ENABLE */
    GpioCtrlRegs.GPBQSEL2.bit.GPIO54 = 3;
    GpioCtrlRegs.GPBMUX2.bit.GPIO54 = 1;
#else
#error "Invalid SPI MOSI GPIO"
#endif /* CONFIG_FIFO_SPI_MOSI_GPIOxx */

    /*
     * Configure MISO pin
     */
#if CONFIG_FIFO_SPI_MISO_GPIO17
#if CONFIG_FIFO_SPI_MISO_PULLUP_ENABLE
    GpioCtrlRegs.GPAPUD.bit.GPIO17 = 0;
#else
    GpioCtrlRegs.GPAPUD.bit.GPIO17 = 1;
#endif /* CONFIG_FIFO_SPI_MISO_PULLUP_ENABLE*/
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 1;
#elif CONFIG_FIFO_SPI_MISO_GPIO55
#if CONFIG_FIFO_SPI_MISO_PULLUP_ENABLE
    GpioCtrlRegs.GPBPUD.bit.GPIO55 = 0;
#else
    GpioCtrlRegs.GPBPUD.bit.GPIO55 = 1;
#endif /* CONFIG_FIFO_SPI_MISO_PULLUP_ENABLE*/
    GpioCtrlRegs.GPBQSEL2.bit.GPIO55 = 3;
    GpioCtrlRegs.GPBMUX2.bit.GPIO55 = 1;
#else
#error "Invalid SPI MISO GPIO"
#endif /* CONFIG_FIFO_SPI_MISO_GPIOxx */

    /*
     * Configure CLOCK pin
     */
#if CONFIG_FIFO_SPI_CLK_GPIO18
#if CONFIG_FIFO_SPI_CLOCK_PULLUP_ENABLE
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0;
#else
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 1;
#endif /* CONFIG_FIFO_SPI_CLOCK_PULLUP_ENABLE */
    GpioCtrlRegs.GPAQSEL2.bit.GPIO18 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;
#elif CONFIG_FIFO_SPI_CLK_GPIO56
#if CONFIG_FIFO_SPI_CLOCK_PULLUP_ENABLE
    GpioCtrlRegs.GPBPUD.bit.GPIO56 = 0;
#else
    GpioCtrlRegs.GPBPUD.bit.GPIO56 = 1;
#endif /* CONFIG_FIFO_SPI_CLOCK_PULLUP_ENABLE */
    GpioCtrlRegs.GPBQSEL2.bit.GPIO56 = 3;
    GpioCtrlRegs.GPBMUX2.bit.GPIO56 = 1;
#else
#error "Invalid SPI Clock GPIO"
#endif /* CONFIG_FIFO_SPI_CLK_GPIOxx */

    EDIS;
}


static UInt16 txIdx = 0;
static UInt16 rxIdx = 0;

/*!
 * SPI Interrupt Handler
 * This is called from SYS/BIOS interrupt dispatcher
 */
#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(spi_tx_fifo_hwi_handler, "ramfuncs");
#endif
static Void spi_tx_fifo_hwi_handler(UArg arg)
{
    SPI_BUS_T id = (SPI_BUS_T)arg;

    SpiaRegs.SPIFFTX.bit.TXFFIENA = 0;
    SpiaRegs.SPIFFTX.bit.TXFFINTCLR = 1;
    if(spi_evt[id] != NULL) {
        Event_post(spi_evt[id], SPI_EVENT_TX_HWI);
    }
}


/*!
 * SPI Interrupt Handler
 * This is called from SYS/BIOS interrupt dispatcher
 */
#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(spi_rx_fifo_hwi_handler, "ramfuncs");
#endif
static Void spi_rx_fifo_hwi_handler(UArg arg)
{
    UInt16 i = 0;
    UInt16 cnt = 0;
    SPI_BUS_T id = (SPI_BUS_T)arg;

    cnt = SpiaRegs.SPIFFRX.bit.RXFFST;
    for(i = 0; i < cnt; i++) {
        spi_transaction_request[id].pBuf[rxIdx] = (SpiaRegs.SPIRXBUF & 0x00FF);
        rxIdx++;
    }

    SpiaRegs.SPIFFRX.bit.RXFFOVFCLR = 1;
    SpiaRegs.SPIFFRX.bit.RXFFINTCLR = 1;

    if(spi_evt[id] != NULL) {
        Event_post(spi_evt[id], SPI_EVENT_RX_HWI);
    }
}


#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(tsk_spi_fifo, "ramfuncs");
#endif
static void tsk_spi_fifo(UArg a0, UArg a1)
{
    SPI_BUS_T id = (SPI_BUS_T)a0;
    UInt16 i = 0;
    UInt posted = 0;

    while(!bInitDone) {
        /* Wait till initialized */
        Task_sleep(1);
    }

#if (CONFIG_ENABLE_CLI_SPI_COMMAND && CONFIG_ENABLE_CLI_SPI_CMD_TRANSACT)
    CMD_SPI_init();
#endif

    while(1) {
        if(Mailbox_pend(mbx_spi[id], &spi_transaction_request[id], BIOS_WAIT_FOREVER)) {
            rxIdx = 0;
            txIdx = 0;
            /* Check request validity */
            if((spi_transaction_request[id].len == 0) ||
               (spi_transaction_request[id].pBuf == NULL) ||
               (spi_transaction_request[id].mode >= N_SPI_MODE)) {
                /* Invalid Request */
                if(spi_transaction_request[id].pStatus != NULL) {
                    *(spi_transaction_request[id].pStatus) = FALSE;
                }
                if(spi_transaction_request[id].semRequestor != NULL) {
                    Semaphore_post(spi_transaction_request[id].semRequestor);
                }
                continue;
            }

            /*
             * Valid Request.
             * Process this request
             */
            SpiaRegs.SPICCR.bit.SPISWRESET = 0;  // Initialize operaating flags to reset condition
            switch(spi_transaction_request[id].width) {
                case SPI_DATA_WIDTH_8: {
                    SpiaRegs.SPICCR.bit.SPICHAR = FIFO_SPI_DATA_WIDTH_8;
                    break;
                }
                case SPI_DATA_WIDTH_16: {
                    SpiaRegs.SPICCR.bit.SPICHAR = FIFO_SPI_DATA_WIDTH_16;
                    break;
                }
                default: {
                    break;
                }
            }
            SpiaRegs.SPICTL.bit.SPIINTENA = 0;
            switch(spi_transaction_request[id].mode) {
                case SPI_MODE0: {
                    SpiaRegs.SPICCR.bit.CLKPOLARITY = 0;
                    SpiaRegs.SPICTL.bit.CLK_PHASE = 1;
                    break;
                }
                case SPI_MODE1: {
                    SpiaRegs.SPICCR.bit.CLKPOLARITY = 0;
                    SpiaRegs.SPICTL.bit.CLK_PHASE = 0;
                    break;
                }
                case SPI_MODE2: {
                    SpiaRegs.SPICCR.bit.CLKPOLARITY = 1;
                    SpiaRegs.SPICTL.bit.CLK_PHASE = 1;
                    break;
                }
                case SPI_MODE3: {
                    SpiaRegs.SPICCR.bit.CLKPOLARITY = 1;
                    SpiaRegs.SPICTL.bit.CLK_PHASE = 0;
                    break;
                }
                default: {
                    /* default to mode 0 */
                    SpiaRegs.SPICCR.bit.CLKPOLARITY = 0;
                    SpiaRegs.SPICTL.bit.CLK_PHASE = 1;
                    break;
                }
            }
            // Reset TX FIFO and RX FIFO
            SpiaRegs.SPIFFTX.bit.SPIRST = 0;
            SpiaRegs.SPIFFTX.bit.TXFIFO = 0;        // Reset Tx FIFO pointer to zero
            SpiaRegs.SPIFFRX.bit.RXFIFORESET = 0;   // Reset Rx FIFO pointer to zero
            SpiaRegs.SPIFFTX.bit.TXFFIENA = 0;
            SpiaRegs.SPIFFRX.bit.RXFFIENA = 0;
            // Select Device
            if(spi_transaction_request[id].fcnCS_cb != NULL) {
                spi_transaction_request[id].fcnCS_cb(TRUE);
            }
            // Push to Tx FIFO
            SpiaRegs.SPIFFTX.bit.SPIRST = 1;
            SpiaRegs.SPICCR.bit.SPISWRESET = 1;
            SpiaRegs.SPIFFTX.bit.TXFIFO = 1;
            SpiaRegs.SPIFFRX.bit.RXFIFORESET = 1;
            if(spi_transaction_request[id].len <= 16) {
                /* Transaction can fit into TX FIFO */
                for (i = 0; i < spi_transaction_request[id].len; i++) {
                    SpiaRegs.SPITXBUF = spi_transaction_request[id].pBuf[i] << 8;  // Must be right-aligned when width is less than 16 bits
                    txIdx++;
                }
                /* Only enable RX FIFO Interrupt */
                while(Event_pend(spi_evt[id], SPI_EVENT_RX_HWI, 0, BIOS_NO_WAIT));
                SpiaRegs.SPIFFRX.bit.RXFFIL = spi_transaction_request[id].len;
                SpiaRegs.SPIFFRX.bit.RXFFINTCLR = 1;
                SpiaRegs.SPIFFRX.bit.RXFFIENA = 1;
                SpiaRegs.SPICTL.bit.SPIINTENA = 1;
                /* Wait for RX FIFO event */
                posted = Event_pend(spi_evt[id], SPI_EVENT_RX_HWI, 0, CONFIG_FIFO_SPI_DEFAULT_TIMEOUT);
                // De-Select Device
                if(spi_transaction_request[id].fcnCS_cb != NULL) {
                    spi_transaction_request[id].fcnCS_cb(FALSE);
                }
                if(spi_transaction_request[id].pStatus != NULL) {
                    *(spi_transaction_request[id].pStatus) = (posted != 0);
                }
                /* Post to requestor */
                if(spi_transaction_request[id].semRequestor != NULL) {
                    Semaphore_post(spi_transaction_request[id].semRequestor);
                }
            } else {
                UInt16 remaining = spi_transaction_request[id].len;
                /* Multiple FIFO transactions */
                while(remaining > 0) {
                    for(i = 0; i < 16; i++) {
                        SpiaRegs.SPITXBUF = spi_transaction_request[id].pBuf[txIdx] << 8;
                        txIdx++;
                        remaining--;
                        if(remaining == 0) {
                            i++;
                            break;
                        }
                    }
                    while(Event_pend(spi_evt[id], 0, SPI_EVENT_RX_HWI | SPI_EVENT_TX_HWI, BIOS_NO_WAIT));
                    SpiaRegs.SPIFFTX.bit.TXFFIL = 0;
                    SpiaRegs.SPIFFTX.bit.TXFFINTCLR = 1;
                    SpiaRegs.SPIFFTX.bit.TXFFIENA = 1;
                    SpiaRegs.SPIFFRX.bit.RXFFIL = i;
                    SpiaRegs.SPIFFRX.bit.RXFFINTCLR = 1;
                    SpiaRegs.SPIFFRX.bit.RXFFIENA = 1;
                    SpiaRegs.SPICTL.bit.SPIINTENA = 1;
                    /* Wait for RX and TX FIFO event */
                    posted = Event_pend(spi_evt[id], SPI_EVENT_RX_HWI | SPI_EVENT_TX_HWI, 0, DEFAULT_TIMEOUT[id]);
                    if(posted == 0) {
                        /* Timed out Error */
                        // De-Select Device
                        if(spi_transaction_request[id].fcnCS_cb != NULL) {
                            spi_transaction_request[id].fcnCS_cb(FALSE);
                        }
                        if(spi_transaction_request[id].pStatus != NULL) {
                            *(spi_transaction_request[id].pStatus) = FALSE;
                        }
                        /* Post to requestor */
                        if(spi_transaction_request[id].semRequestor != NULL) {
                            Semaphore_post(spi_transaction_request[id].semRequestor);
                        }
                        break;
                    } else {
                        if(remaining == 0) {
                            // De-Select Device
                            if(spi_transaction_request[id].fcnCS_cb != NULL) {
                                spi_transaction_request[id].fcnCS_cb(FALSE);
                            }
                            if(spi_transaction_request[id].pStatus != NULL) {
                                *(spi_transaction_request[id].pStatus) = TRUE;
                            }
                            /* Post to requestor */
                            if(spi_transaction_request[id].semRequestor != NULL) {
                                Semaphore_post(spi_transaction_request[id].semRequestor);
                            }
                        }
                    }
                }
            }
        }
    }
}
#endif /* CONFIG_ENABLE_FIFO_SPI */

#if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI)
static void spi_delay_loop(Int32 dlyCnt)
{
    while(dlyCnt > 0) {
        dlyCnt--;
    }
}
#endif /* (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI) */


SPI_RET_T SPI_init(void)
{
#if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI || CONFIG_ENABLE_FIFO_SPI)
    Error_Block eb;
    Mailbox_Params mbxParams;
    Task_Params tskParams;
    Hwi_Params hwiParams;

    if(!bInitDone) {
        Error_init(&eb);

#if CONFIG_ENABLE_MCBSPA_SPI
        // Configure McBSP-A pins
        EALLOW;
        SysCtrlRegs.PCLKCR0.bit.MCBSPAENCLK = 1;
        GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 2;    // MDXA pin
        GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 2;    // MDRA pin
        GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 2;    // MCLKXA pin
#if CONFIG_MCBSPA_SPI_MDX_PULLUP_ENABLE
        GpioCtrlRegs.GPAPUD.bit.GPIO20 = 0;     // Enable pull-up on MDXA
#else
        GpioCtrlRegs.GPAPUD.bit.GPIO20 = 1;     // Disable pull-up on MDXA
#endif /* CONFIG_MCBSPA_SPI_MDX_PULLUP_ENABLE */
#if CONFIG_MCBSPA_SPI_MDR_PULLUP_ENABLE
        GpioCtrlRegs.GPAPUD.bit.GPIO21 = 0;     // Enable pull-up on MDRA
#else
        GpioCtrlRegs.GPAPUD.bit.GPIO21 = 1;     // Disable pull-up on MDRA
#endif /* CONFIG_MCBSPA_SPI_MDR_PULLUP_ENABLE */
#if CONFIG_MCBSPA_SPI_MCLX_PULLUP_ENABLE
        GpioCtrlRegs.GPAPUD.bit.GPIO22 = 0;     // Enable pull-up on MCLKXA
#else
        GpioCtrlRegs.GPAPUD.bit.GPIO22 = 1;     // Disable pull-up on MCLKXA
#endif /* CONFIG_MCBSPA_SPI_MCLX_PULLUP_ENABLE */
        GpioCtrlRegs.GPAQSEL2.bit.GPIO21 = 3;   // Asynch input MDRA
        GpioCtrlRegs.GPAQSEL2.bit.GPIO22 = 3;   // Asynch input MCLKXA
        EDIS;
        /*
         * SPI event
         */
        spi_evt[SPI_BUS_MCBSP_A] = Event_create(NULL, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (spi_evt[SPI_BUS_MCBSP_A] != NULL), NULL);
        /*
         * Mailbox instance
         */
        Mailbox_Params_init(&mbxParams);
        mbx_spi[SPI_BUS_MCBSP_A] = Mailbox_create(sizeof(SPI_REQUEST_MB_OBJ_T),
                            DEFAULT_SPI_MAX_TRANSACTION_REQUEST[SPI_BUS_MCBSP_A],
                            &mbxParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (mbx_spi[SPI_BUS_MCBSP_A] != NULL), NULL);
        /*
         * Task
         */
        Task_Params_init(&tskParams);
        tskParams.priority = DEFAULT_SPI_TASK_PRIORITY[SPI_BUS_MCBSP_A];
        tskParams.stackSize = DEFAULT_SPI_TASK_STACK[SPI_BUS_MCBSP_A];
        tskParams.arg0 = SPI_BUS_MCBSP_A;
        tskParams.instance->name = tag[SPI_BUS_MCBSP_A];
        tskHdl_spi[SPI_BUS_MCBSP_A] = Task_create(tsk_spi_dma, &tskParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (tskHdl_spi[SPI_BUS_MCBSP_A] != NULL), NULL);
        /*
         * Hwi
         */
        /* DMA Tx Hwi */
        Hwi_Params_init(&hwiParams);
        hwiParams.enableAck = true;
        hwiParams.arg = SPI_BUS_MCBSP_A;
        hwiParams.instance->name = tag_hwi[SPI_BUS_MCBSP_A][0];
        hwiHdl_spi[SPI_BUS_MCBSP_A].hwiTx = Hwi_create(DEFAULT_HWI_INT_NUM[SPI_BUS_MCBSP_A][0],
                                                    spi_tx_dma_hwi_handler, &hwiParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) &&
                (hwiHdl_spi[SPI_BUS_MCBSP_A].hwiTx != NULL), NULL);
        /* DMA Rx Hwi */
        Hwi_Params_init(&hwiParams);
        hwiParams.enableAck = true;
        hwiParams.arg = SPI_BUS_MCBSP_A;
        hwiParams.instance->name = tag_hwi[SPI_BUS_MCBSP_A][1];
        hwiHdl_spi[SPI_BUS_MCBSP_A].hwiRx = Hwi_create(DEFAULT_HWI_INT_NUM[SPI_BUS_MCBSP_A][1],
                                                    spi_rx_dma_hwi_handler, &hwiParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) &&
                (hwiHdl_spi[SPI_BUS_MCBSP_A].hwiRx != NULL), NULL);
        /*
         * Configure McBSP-A
         */
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SPCR2.all = 0x0000;    // Reset FS generator, sample rate generator & transmitter
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SPCR1.all = 0x0000;    // Reset Receiver, Right justify word, Digital loopback disabled
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->MFFINT.all = 0x0000;   // Disable all interrupts
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->RCR2.all = 0x0000;     // Reset RCR2
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->RCR1.all = 0x0000;     // Reset RCR1
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->XCR2.all = 0x0000;     // Reset XCR2
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->XCR1.all = 0x0000;     // Reset XCR1
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->PCR.all = 0x0F08;      // (CLKXM=CLKRM=FSXM=FSRM=1, FSXP=1)
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SPCR1.bit.CLKSTP = 2;  // Together with CLKXP/CLKRP determines clocking scheme
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->PCR.bit.CLKXP = 1;     // CPOL = 1, CPHA = 1 rising edge no delay
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->PCR.bit.CLKRP = 0;
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->RCR2.bit.RDATDLY = 1;  // FSX setup time 1 in master mode. 0 for slave mode (Receive)
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->XCR2.bit.XDATDLY = 1;  // FSX setup time 1 in master mode. 0 for slave mode (Transmit)
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->RCR1.bit.RWDLEN1 = 2;  // 16-bit word
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->XCR1.bit.XWDLEN1 = 2;  // 16-bit word
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SRGR2.all = 0x2000;    // CLKSM=1, FPER=1 CLKG periods
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SRGR1.all = 0x0002;    // CLKG = (150MHz/4) / (CLKGDV + 1) = 12.5MHz
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SPCR2.bit.GRST = 1;    // Enable the sample rate generator
        spi_delay_loop(8);                          // Wait for 2 SRG
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SPCR2.bit.XRST = 1;    // Release TX from Reset
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SPCR1.bit.RRST = 1;    // Release RX from Reset
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_A]->SPCR2.bit.FRST = 1;    // Release Frame Sync from Reset
#endif /* CONFIG_ENABLE_MCBSPA_SPI */

#if CONFIG_ENABLE_MCBSPB_SPI
        // Configure McBSP-B pins
        EALLOW;
        SysCtrlRegs.PCLKCR0.bit.MCBSPBENCLK = 1;
        MCBSPB_SPI_MOSI_MUX_CONFIG;
        MCBSPB_SPI_MISO_MUX_CONFIG;
        MCBSPB_SPI_MCLK_MUX_CONFIG;
        MCBSPB_SPI_MOSI_PULLUP_CONFIG;
        MCBSPB_SPI_MISO_PULLUP_CONFIG;
        MCBSPB_SPI_MCLK_PULLUP_CONFIG;
        MCBSPB_SPI_MOSI_SYNC_CONFIG;
        MCBSPB_SPI_MISO_SYNC_CONFIG;
        EDIS;
        /*
         * SPI event
         */
        spi_evt[SPI_BUS_MCBSP_B] = Event_create(NULL, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (spi_evt[SPI_BUS_MCBSP_B] != NULL), NULL);
        /*
         * Mailbox instance
         */
        Mailbox_Params_init(&mbxParams);
        mbx_spi[SPI_BUS_MCBSP_B] = Mailbox_create(sizeof(SPI_REQUEST_MB_OBJ_T),
                            DEFAULT_SPI_MAX_TRANSACTION_REQUEST[SPI_BUS_MCBSP_B],
                            &mbxParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (mbx_spi[SPI_BUS_MCBSP_B] != NULL), NULL);
        /*
         * Task
         */
        Task_Params_init(&tskParams);
        tskParams.priority = DEFAULT_SPI_TASK_PRIORITY[SPI_BUS_MCBSP_B];
        tskParams.stackSize = DEFAULT_SPI_TASK_STACK[SPI_BUS_MCBSP_B];
        tskParams.arg0 = SPI_BUS_MCBSP_B;
        tskParams.instance->name = tag[SPI_BUS_MCBSP_B];
        tskHdl_spi[SPI_BUS_MCBSP_B] = Task_create(tsk_spi_dma, &tskParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (tskHdl_spi[SPI_BUS_MCBSP_B] != NULL), NULL);
        /*
         * Hwi
         */
        /* DMA Tx Hwi */
        Hwi_Params_init(&hwiParams);
        hwiParams.enableAck = true;
        hwiParams.arg = SPI_BUS_MCBSP_B;
        hwiParams.instance->name = tag_hwi[SPI_BUS_MCBSP_B][0];
        hwiHdl_spi[SPI_BUS_MCBSP_B].hwiTx = Hwi_create(DEFAULT_HWI_INT_NUM[SPI_BUS_MCBSP_B][0],
                                                    spi_tx_dma_hwi_handler, &hwiParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) &&
                (hwiHdl_spi[SPI_BUS_MCBSP_B].hwiTx != NULL), NULL);
        /* DMA Rx Hwi */
        Hwi_Params_init(&hwiParams);
        hwiParams.enableAck = true;
        hwiParams.arg = SPI_BUS_MCBSP_B;
        hwiParams.instance->name = tag_hwi[SPI_BUS_MCBSP_B][1];
        hwiHdl_spi[SPI_BUS_MCBSP_B].hwiRx = Hwi_create(DEFAULT_HWI_INT_NUM[SPI_BUS_MCBSP_B][1],
                                                    spi_rx_dma_hwi_handler, &hwiParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) &&
                (hwiHdl_spi[SPI_BUS_MCBSP_B].hwiRx != NULL), NULL);
        /*
         * Configure McBSP-A
         */
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SPCR2.all = 0x0000;    // Reset FS generator, sample rate generator & transmitter
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SPCR1.all = 0x0000;    // Reset Receiver, Right justify word, Digital loopback disabled
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->MFFINT.all = 0x0000;   // Disable all interrupts
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->RCR2.all = 0x0000;     // Reset RCR2
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->RCR1.all = 0x0000;     // Reset RCR1
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->XCR2.all = 0x0000;     // Reset XCR2
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->XCR1.all = 0x0000;     // Reset XCR1
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->PCR.all = 0x0F08;      // (CLKXM=CLKRM=FSXM=FSRM=1, FSXP=1)
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SPCR1.bit.CLKSTP = 2;  // Together with CLKXP/CLKRP determines clocking scheme
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->PCR.bit.CLKXP = 1;     // CPOL = 1, CPHA = 1 rising edge no delay
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->PCR.bit.CLKRP = 0;
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->RCR2.bit.RDATDLY = 1;  // FSX setup time 1 in master mode. 0 for slave mode (Receive)
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->XCR2.bit.XDATDLY = 1;  // FSX setup time 1 in master mode. 0 for slave mode (Transmit)
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->RCR1.bit.RWDLEN1 = 2;  // 16-bit word
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->XCR1.bit.XWDLEN1 = 2;  // 16-bit word
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SRGR2.all = 0x2000;    // CLKSM=1, FPER=1 CLKG periods
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SRGR1.all = 0x0002;    // CLKG = (150MHz/4) / (CLKGDV + 1) = 12.5MHz
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SPCR2.bit.GRST = 1;    // Enable the sample rate generator
        spi_delay_loop(8);                          // Wait for 2 SRG
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SPCR2.bit.XRST = 1;    // Release TX from Reset
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SPCR1.bit.RRST = 1;    // Release RX from Reset
        MCBSP_REGS_PTR[SPI_BUS_MCBSP_B]->SPCR2.bit.FRST = 1;    // Release Frame Sync from Reset
#endif /* CONFIG_ENABLE_MCBSPB_SPI */

#if CONFIG_ENABLE_FIFO_SPI
        init_spi_fifo_gpio();
        /*
         * SPI event
         */
        spi_evt[SPI_BUS_FIFO] = Event_create(NULL, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (spi_evt[SPI_BUS_FIFO] != NULL), NULL);
        /*
         * Mailbox instance
         */
        Mailbox_Params_init(&mbxParams);
        mbx_spi[SPI_BUS_FIFO] = Mailbox_create(sizeof(SPI_REQUEST_MB_OBJ_T),
                            DEFAULT_SPI_MAX_TRANSACTION_REQUEST[SPI_BUS_FIFO],
                            &mbxParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (mbx_spi[SPI_BUS_FIFO] != NULL), NULL);
        /*
         * Task
         */
        Task_Params_init(&tskParams);
        tskParams.priority = DEFAULT_SPI_TASK_PRIORITY[SPI_BUS_FIFO];
        tskParams.stackSize = DEFAULT_SPI_TASK_STACK[SPI_BUS_FIFO];
        tskParams.arg0 = SPI_BUS_FIFO;
        tskParams.instance->name = tag[SPI_BUS_FIFO];
        tskHdl_spi[SPI_BUS_FIFO] = Task_create(tsk_spi_fifo, &tskParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) && (tskHdl_spi[SPI_BUS_FIFO] != NULL), NULL);
        /*
         * Hwi
         */
        /* FIFO Tx Hwi */
        Hwi_Params_init(&hwiParams);
        hwiParams.enableAck = true;
        hwiParams.arg = SPI_BUS_FIFO;
        hwiParams.instance->name = tag_hwi[SPI_BUS_FIFO][0];
        hwiHdl_spi[SPI_BUS_FIFO].hwiTx = Hwi_create(DEFAULT_HWI_INT_NUM[SPI_BUS_FIFO][0],
                                                    spi_tx_fifo_hwi_handler, &hwiParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) &&
                (hwiHdl_spi[SPI_BUS_FIFO].hwiTx != NULL), NULL);
        /* FIFO Rx Hwi */
        Hwi_Params_init(&hwiParams);
        hwiParams.enableAck = true;
        hwiParams.arg = SPI_BUS_FIFO;
        hwiParams.instance->name = tag_hwi[SPI_BUS_FIFO][1];
        hwiHdl_spi[SPI_BUS_FIFO].hwiRx = Hwi_create(DEFAULT_HWI_INT_NUM[SPI_BUS_FIFO][1],
                                                    spi_rx_fifo_hwi_handler, &hwiParams, &eb);
        Assert_isTrue((Error_check(&eb) == FALSE) &&
                (hwiHdl_spi[SPI_BUS_FIFO].hwiRx != NULL), NULL);
        /*
         * Initialize SPI peripheral
         */
        SpiaRegs.SPICCR.all = 0;
        SpiaRegs.SPICCR.bit.SPICHAR = FIFO_SPI_DATA_WIDTH_8;
        SpiaRegs.SPICTL.bit.MASTER_SLAVE = 0x1; /* Configured as Master */
        SpiaRegs.SPICCR.bit.CLKPOLARITY = 0;
        SpiaRegs.SPICTL.bit.CLK_PHASE = 1;
        SpiaRegs.SPICTL.bit.TALK = 1;
        SpiaRegs.SPIBRR = FIFO_SPI_BRR_VALUE;
        // Hold FIFO in reset
        SpiaRegs.SPIFFTX.bit.SPIRST = 0;
        SpiaRegs.SPIFFTX.bit.SPIFFENA = 1;  // FIFO mode
        SpiaRegs.SPIFFTX.bit.TXFIFO = 0;
        SpiaRegs.SPIFFRX.bit.RXFIFORESET = 0;
        SpiaRegs.SPIFFCT.bit.TXDLY = 0;
        /* Leave interrupt disabled */
        SpiaRegs.SPICTL.bit.SPIINTENA = 0;
        SpiaRegs.SPICCR.bit.SPISWRESET = 1;
#endif /* CONFIG_ENABLE_FIFO_SPI */

#if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI)
        EALLOW;
        SysCtrlRegs.PCLKCR3.bit.DMAENCLK = 1;       // DMA Clock
        DmaRegs.DMACTRL.bit.HARDRESET = 1;
        __asm(" NOP");
        EDIS;
#endif /* (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI) */
        bInitDone = true;
    }
#endif /* #if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI || CONFIG_ENABLE_FIFO_SPI) */

    return SPI_OK;
}


bool SPI_init_done(void)
{
    return (bInitDone);
}


#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(SPI_transact, "ramfuncs");
#endif
static SPI_RET_T SPI_transact(SPI_BUS_T id, SPI_DATA_WIDTH_T width, UInt16 * pBuf, UInt16 len, SPI_MODE_T mode, Bool * pResult, Semaphore_Handle sem, SPI_setChipSelect_cb fcn_cs)
{
    SPI_REQUEST_MB_OBJ_T spi_transaction_instance;

    if(!bInitDone) {
        return SPI_INVALID_STATE;
    }

    if((id >= N_SPI_BUS) || (pBuf == NULL) || (len == 0)) {
        return SPI_INVALID_ARG;
    }
    /*
     * If using DMA, check valid Memory location
     */
    switch(id) {
#if (CONFIG_ENABLE_MCBSPA_SPI)
        case SPI_BUS_MCBSP_A:
#endif
#if (CONFIG_ENABLE_MCBSPB_SPI)
        case SPI_BUS_MCBSP_B:
#endif
#if (CONFIG_ENABLE_MCBSPA_SPI || CONFIG_ENABLE_MCBSPB_SPI)
        {
            if(((UInt32)pBuf < 0x00C000) || (((UInt32)pBuf + len) > 0x00FFFF)) {
                /* Buffer is not located in L4-L7 SARAM (DMA) */
                return SPI_INVALID_ARG;
            }
            break;
        }
#endif
        default: {
            break;
        }
    }

    spi_transaction_instance.pBuf = pBuf;
    spi_transaction_instance.len = len;
    spi_transaction_instance.width = width;
    spi_transaction_instance.mode = mode;
    spi_transaction_instance.pStatus = pResult;
    spi_transaction_instance.semRequestor = sem;
    spi_transaction_instance.fcnCS_cb = fcn_cs;

    if(TRUE != Mailbox_post(mbx_spi[id], &spi_transaction_instance, BIOS_NO_WAIT)) {
        return SPI_ERR_MBX_FULL;
    }

    return SPI_OK;
}


#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(SPI_transact_8bit, "ramfuncs");
#endif
SPI_RET_T SPI_transact_8bit(SPI_BUS_T id, UInt8 * pBuf, UInt16 len, SPI_MODE_T mode, Bool * pResult, Semaphore_Handle sem, SPI_setChipSelect_cb fcn_cs)
{
    return (SPI_transact(id, SPI_DATA_WIDTH_8, (UInt16 *)pBuf, len, mode, pResult, sem, fcn_cs));
}


#if CONFIG_SPI_RUN_IN_RAM
#pragma CODE_SECTION(SPI_transact_16bit, "ramfuncs");
#endif
SPI_RET_T SPI_transact_16bit(SPI_BUS_T id, UInt16 * pBuf, UInt16 len, SPI_MODE_T mode, Bool * pResult, Semaphore_Handle sem, SPI_setChipSelect_cb fcn_cs)
{
    return (SPI_transact(id, SPI_DATA_WIDTH_16, (UInt16 *)pBuf, len, mode, pResult, sem, fcn_cs));
}

#endif /* (CONFIG_USE_SPI_DEVICE) */


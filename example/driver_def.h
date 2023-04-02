#ifndef DRIVER_DEF_H
#define DRIVER_DEF_H

#define CONFIG_SYSTEM_FREQ_MHZ  150

/*
 * UART Driver Configuration
 */
#define CONFIG_USE_UART                     1
#define CONFIG_ENABLE_UARTA                 1
#if CONFIG_ENABLE_UARTA
#define CONFIG_BAUDRATE_UARTA               115200
#define CONFIG_TX_QUEUE_BUFF_SZ_UARTA       1024
#define CONFIG_RX_QUEUE_BUFF_SZ_UARTA       128
#endif

#define CONFIG_ENABLE_UARTB                 0
#if CONFIG_ENABLE_UARTB
#define CONFIG_BAUDRATE_UARTB               115200
#define CONFIG_TX_QUEUE_BUFF_SZ_UARTB       256
#define CONFIG_RX_QUEUE_BUFF_SZ_UARTB       128
#endif

#define CONFIG_ENABLE_UARTC                 0
#if CONFIG_ENABLE_UARTC
#define CONFIG_BAUDRATE_UARTC               115200
#define CONFIG_TX_QUEUE_BUFF_SZ_UARTC       256
#define CONFIG_RX_QUEUE_BUFF_SZ_UARTC       128
#endif

/*
 * Command Line Console Configuration
 */
#define CONFIG_USE_CLI                      1
#define CONFIG_CLI_TASK_PRIORITY            1
#define CONFIG_CLI_TASK_STACK               2048
#if CONFIG_USE_UART
#define CONFIG_CLI_IO_UART                  1
  #if CONFIG_ENABLE_UARTA
    #define CONFIG_CLI_IO_UART_A            1
  #endif
  #if CONFIG_ENABLE_UARTB
    #define CONFIG_CLI_IO_UART_B            1
  #endif
  #if CONFIG_ENABLE_UARTC
    #define CONFIG_CLI_IO_UART_C            1
  #endif
#endif /* CONFIG_USE_UART */
#define CONFIG_CLI_COMMAND_MAX_OUTPUT_SIZE  256
#define CONFIG_CLI_COMMAND_MAX_INPUT_SIZE   256
#define CONFIG_SYSBIOS_PRINTF_UART_A        1
#define CONFIG_SYSBIOS_PRINTF_UART_B        1
#define CONFIG_SYSBIOS_PRINTF_UART_C        1

/*
 * SPI Configuration
 */
#define CONFIG_USE_SPI_DEVICE               1
#define CONFIG_SPI_RUN_IN_RAM               1
#define CONFIG_ENABLE_MCBSPA_SPI            0
#define CONFIG_ENABLE_MCBSPB_SPI            1
#if CONFIG_ENABLE_MCBSPB_SPI
  #define CONFIG_MCBSPB_SPI_MCLKX_GPIO      14
  #define CONFIG_MCBSPB_SPI_MDX_GPIO        12
  #define CONFIG_MCBSPB_SPI_MDR_GPIO        13
  #define CONFIG_TASK_PRIO_MCBSPB_SPI       1
  #define CONFIG_TASK_STACK_MCBSPB_SPI      512
  #define CONFIG_MCBSPB_SPI_MAX_REQUEST_COUNT   10
  #define CONFIG_MCBSPB_SPI_DEFAULT_TIMEOUT 100
#endif /* CONFIG_ENABLE_MCBSPB_SPI */
#define CONFIG_ENABLE_CLI_SPI_COMMAND       0
#if CONFIG_ENABLE_CLI_SPI_COMMAND
  #define CONFIG_ENABLE_CLI_SPI_CMD_TRANSACT 1
  #define CONFIG_ENABLE_CLI_SPI_CMD_LIST    1
  #if CONFIG_ENABLE_MCBSPB_SPI
    #define CONFIG_CLI_SPI_MCBSPB_CS_GPIO   15
  #endif /* CONFIG_ENABLE_MCBSPB_SPI */
#endif /* CONFIG_ENABLE_CLI_SPI_COMMAND */

/*
 * FRAM (FM25W256) Configuration
 */
#define CONFIG_USE_SPI_FM25W256             1
#define CONFIG_SPI_FM25W256_MCBSPA          0
#define CONFIG_SPI_FM25W256_MCBSPB          1
#define CONFIG_SPI_FM25W256_CS              15
#define CONFIG_SPI_FM25W256_SIZE            32768
#define CONFIG_SPI_FM25W256_DEFAULT_TIMEOUT 100
#define CONFIG_ENABLE_CLI_FM25W256_COMMAND  1
#define CONFIG_ENABLE_CLI_FM25W256_CMD_WRITE 1
#define CONFIG_ENABLE_CLI_FM25W256_CMD_READ 1

/*
 * LittleFS Configuration
 */
#define CONFIG_FRAM_LFS_READ_SZ             1
#define CONFIG_FRAM_LFS_PROG_SZ             1
#define CONFIG_FRAM_LFS_BLOCK_SZ            128
#define CONFIG_ENABLE_CLI_LFS_COMMAND       1
#define CONFIG_ENABLE_CLI_LFS_CMD_FORMAT    1

#endif /* DRIVER_DEF_H */

/*!
 * \file fm25w256.h
 */
#ifndef FM25W256_H
#define FM25W256_H

#include "autoconf.h"

#if CONFIG_USE_SPI_FM25W256

typedef enum {
    FM25W256_OK = 0,        /*!< No Error */
    FM25W256_INVALID_ARG,   /*!< Invalid argument */
    FM25W256_INVALID_STATE, /*!< Not in a valid state */
    FM25W256_TIMEOUT,       /*!< Timed Out */
    FM25W256_ERROR,         /*!< General Error */
} FM25W256_RET_T;

#define FM25W256_OPCODE_OFFSET  (0)
#define FM25W256_OPCODE_SZ      (1)
#define FM25W256_ADDR_OFFSET    (FM25W256_OPCODE_OFFSET + FM25W256_OPCODE_SZ)
#define FM25W256_ADDR_SZ        (2)
#define FM25W256_DATA_OFFSET    (FM25W256_ADDR_OFFSET + FM25W256_ADDR_SZ)

#define FM25W256_DECLARE_BUFFER(varname, sz)   static UInt8 varname[sz + FM25W256_OPCODE_SZ + FM25W256_ADDR_SZ]

#define FM25W256_SET_ADRESS(varname, addr)  varname[FM25W256_ADDR_OFFSET] = (addr >> 8) & 0x00FF; \
                                            varname[FM25W256_ADDR_OFFSET + 1] = addr & 0x00FF

#define FM25W256_BUFFER(varname, offset)   varname[FM25W256_DATA_OFFSET + offset]

FM25W256_RET_T FM25W256_init(void);
FM25W256_RET_T FM25W256_write(UInt8 * pBuf, UInt16 len);
FM25W256_RET_T FM25W256_read(UInt8 * pBuf, UInt16 len);

#endif /* CONFIG_USE_SPI_FM25W256 */
#endif /* FM25W256_H */

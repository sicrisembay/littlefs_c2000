/*!
 * \file lfs_c2000_fram.c
 */

#include "driver_def.h"
#include "lfs.h"
#if CONFIG_USE_SPI_FM25W256
#include "fram/fm25w256/fm25w256.h"
#endif /* CONFIG_USE_SPI_FM25W256 */

static struct lfs_config cfg;
static lfs_t lfs;
static int bInit = 0;

#if CONFIG_USE_SPI_FM25W256
#pragma DATA_SECTION(fm25w256Buffer, "DMARAM")
FM25W256_DECLARE_BUFFER(fm25w256Buffer, CONFIG_FRAM_LFS_BLOCK_SZ);
#endif /* CONFIG_USE_SPI_FM25W256 */

static int fram_read(const struct lfs_config *c,
                    lfs_block_t block,
                    lfs_off_t off,
                    void *buffer,
                    lfs_size_t size)
{
    int ret = LFS_ERR_OK;
    uint32_t address = 0;
    uint16_t i = 0;
#if CONFIG_USE_SPI_FM25W256
    FM25W256_RET_T retFram = FM25W256_OK;
    memset(fm25w256Buffer, 0xFF, sizeof(fm25w256Buffer));
    address = (block * c->block_size) + off;
    FM25W256_SET_ADRESS(fm25w256Buffer, address);
    retFram = FM25W256_read(fm25w256Buffer, size + FM25W256_OPCODE_SZ + FM25W256_ADDR_SZ);
    if(FM25W256_OK != retFram) {
        ret = LFS_ERR_IO;
    } else {
        for(i = 0; i < size; i++) {
            ((uint8_t *)buffer)[i] = FM25W256_BUFFER(fm25w256Buffer, i);
        }
    }
#endif /* CONFIG_USE_SPI_FM25W256 */
    return (ret);
}


static int fram_prog(const struct lfs_config *c,
                    lfs_block_t block,
                    lfs_off_t off,
                    const void *buffer,
                    lfs_size_t size)
{
    int ret = LFS_ERR_OK;
    uint32_t address = 0;
    uint16_t i = 0;
#if CONFIG_USE_SPI_FM25W256
    FM25W256_RET_T retFram = FM25W256_OK;
    address = (block * c->block_size) + off;
    FM25W256_SET_ADRESS(fm25w256Buffer, address);
    for(i = 0; i < size; i++) {
        FM25W256_BUFFER(fm25w256Buffer, i) = ((uint8_t *)buffer)[i];
    }
    retFram = FM25W256_write(fm25w256Buffer, size + FM25W256_OPCODE_SZ + FM25W256_ADDR_SZ);
    if(FM25W256_OK != retFram) {
        ret = LFS_ERR_IO;
    }
#endif /* CONFIG_USE_SPI_FM25W256 */
    return (ret);
}


static int fram_erase(const struct lfs_config *c, lfs_block_t block)
{
    int ret = LFS_ERR_OK;
    uint32_t address = 0;
    uint16_t i = 0;
#if CONFIG_USE_SPI_FM25W256
    FM25W256_RET_T retFram = FM25W256_OK;
    address = block * c->block_size;
    FM25W256_SET_ADRESS(fm25w256Buffer, address);
    for(i = 0; i < c->block_size; i++) {
        FM25W256_BUFFER(fm25w256Buffer, i) = 0xFF;
    }
    retFram = FM25W256_write(fm25w256Buffer, c->block_size + FM25W256_OPCODE_SZ + FM25W256_ADDR_SZ);
    if(FM25W256_OK != retFram) {
        ret = LFS_ERR_IO;
    }
#endif /* CONFIG_USE_SPI_FM25W256 */
    return (ret);
}


static int fram_sync(const struct lfs_config *c)
{
    return 0;
}


#define CONFIG_FRAM_LFS_CACHE_SIZE      (64)
#define CONFIG_FRAM_LFS_BLOCK_COUNT     (CONFIG_SPI_FM25W256_SIZE / CONFIG_FRAM_LFS_BLOCK_SZ)
#define CONFIG_FRAM_LFS_LOOKAHEAD_SIZE  (CONFIG_FRAM_LFS_BLOCK_COUNT / 8)

static uint8_t lfs_readBuffer[CONFIG_FRAM_LFS_CACHE_SIZE];
static uint8_t lfs_progBuffer[CONFIG_FRAM_LFS_CACHE_SIZE];
static uint8_t lfs_lookAheadBuffer[CONFIG_FRAM_LFS_LOOKAHEAD_SIZE];

static void lfs_c2000_init()
{
    if(bInit == 0) {
        memset(&cfg, 0, sizeof(cfg));
        cfg.context = NULL;
        cfg.read = fram_read;
        cfg.prog = fram_prog;
        cfg.erase = fram_erase;
        cfg.sync = fram_sync;
        cfg.read_size = CONFIG_FRAM_LFS_READ_SZ;
        cfg.prog_size = CONFIG_FRAM_LFS_PROG_SZ;
        cfg.block_size = CONFIG_FRAM_LFS_BLOCK_SZ;
        cfg.block_count = CONFIG_FRAM_LFS_BLOCK_COUNT;
        cfg.block_cycles = 500;
        cfg.cache_size = CONFIG_FRAM_LFS_CACHE_SIZE;
        cfg.lookahead_size = CONFIG_FRAM_LFS_LOOKAHEAD_SIZE;

        cfg.read_buffer = lfs_readBuffer;
        cfg.prog_buffer = lfs_progBuffer;
        cfg.lookahead_buffer = lfs_lookAheadBuffer;
        bInit = 1;
    }
}


int lfs_c2000_format()
{
    if(bInit == 0) {
        lfs_c2000_init();
    }
    return (lfs_format(&lfs, &cfg));
}


// Software CRC implementation with small lookup table
uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size) {
    static const uint32_t rtable[16] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
    };

    const uint8_t *data = buffer;

    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 0)) & 0xf];
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 4)) & 0xf];
    }

    return crc;
}

/*
 * ESP32 YModem driver
 *
 * Copyright (C) LoBo 2017
 *
 * Author: Boris Lovosevic (loboris@gmail.com)
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *
 * Date: 2023-April-24
 * sicris.embay@gmail.com
 *     Adopted for C2000 MCU and LittleFS
 *
 */

#include "autoconf.h"

#if CONFIG_USE_YMODEM

#ifndef int8_t
#define int8_t int_least8_t
#endif

#ifndef uint8_t
#define uint8_t uint_least8_t
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/sysbios/knl/Task.h>

#include "ymodem.h"
#include "uart/uart.h"
#if CONFIG_USE_LITTLEFS
#include "lfs.h"
#endif

#if CONFIG_YMODEM_UART_A
#define YMODEM_UART     UART_A
#elif CONFIG_YMODEM_UART_B
#define YMODEM_UART     UART_B
#elif CONFIG_YMODEM_UART_C
#define YMODEM_UART     UART_C
#endif

#if CONFIG_USE_LITTLEFS
static lfs_t * pLfs = NULL;
static lfs_file_t * pFile = NULL;
#endif

//------------------------------------------------------------------------
static unsigned short crc16(const uint8_t *buf, unsigned long count)
{
  uint16_t crc = 0;
  int i;

  while(count--) {
    crc = crc ^ *buf++ << 8;

    for (i=0; i<8; i++) {
      if (crc & 0x8000) crc = crc << 1 ^ 0x1021;
      else crc = crc << 1;
    }
  }
  return crc;
}

//--------------------------------------------------------------
static int32_t Receive_Byte (uint8_t * c, uint32_t timeout)
{
    Char ch;
    uint16_t len = 0;
    len = UART_receive(YMODEM_UART, &ch, 1);

    while((len <= 0) && (timeout > 0)) {
        timeout--;
        Task_sleep(1);
        len = UART_receive(YMODEM_UART, &ch, 1);
    }
    if (len <= 0) {
        return -1;
    }

    *c = *((unsigned char *)&ch);
    return 0;
}

//------------------------
static void uart_consume()
{
    Char ch;
    int16_t timeout = 100;
    uint16_t len = 0;

    /* Empty UART (no new receive within 100ms timeout) */
    while(timeout > 0) {
        len = UART_receive(YMODEM_UART, &ch, 1);
        if(len > 0) {
            timeout = 100;
        } else {
            timeout--;
        }
        Task_sleep(1);
    }
}

//--------------------------------
static uint32_t Send_Byte (char c)
{
    while(0 == UART_send(YMODEM_UART, &c, 1)) {
        /* Unable to send, retry */
        Task_sleep(1);
    }
    while(!UART_send_done(YMODEM_UART)) {
        Task_sleep(1);
    }
    return 0;
}

//----------------------------
static void send_CA ( void ) {
    uint16_t timeout = 100;
    Send_Byte(CA);
    Send_Byte(CA);
    while(!UART_send_done(YMODEM_UART)) {
        Task_sleep(1);
        timeout--;
        if(timeout == 0) {
            break;
        }
    }
}

//-----------------------------
static void send_ACK ( void ) {
    uint16_t timeout = 100;
    Send_Byte(ACK);
    while(!UART_send_done(YMODEM_UART)) {
        Task_sleep(1);
        timeout--;
        if(timeout == 0) {
            break;
        }
    }
}

//----------------------------------
static void send_ACKCRC16 ( void ) {
    uint16_t timeout = 100;
    Send_Byte(ACK);
    Send_Byte(CRC16);
    while(!UART_send_done(YMODEM_UART)) {
        Task_sleep(1);
        timeout--;
        if(timeout == 0) {
            break;
        }
    }
}

//-----------------------------
static void send_NAK ( void ) {
    uint16_t timeout = 100;
    Send_Byte(NAK);
    while(!UART_send_done(YMODEM_UART)) {
        Task_sleep(1);
        timeout--;
        if(timeout == 0) {
            break;
        }
    }
}

//-------------------------------
static void send_CRC16 ( void ) {
    uint16_t timeout = 100;
    Send_Byte(CRC16);
    while(!UART_send_done(YMODEM_UART)) {
        Task_sleep(1);
        timeout--;
        if(timeout == 0) {
            break;
        }
    }
}


/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  timeout
  * @param  length
  *    >0: packet length
  *     0: end of transmission
  *    -1: abort by sender
  *    -2: error or crc error
  * @retval 0: normally return
  *        -1: timeout
  *        -2: abort by user
  */
//--------------------------------------------------------------------------
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
    int32_t count, packet_size, i;
    uint8_t ch;
    *length = 0;
  
    // receive 1st byte
    if (Receive_Byte(&ch, timeout) < 0) {
        return -1;
    }

    switch (ch) {
    case SOH:
        packet_size = PACKET_SIZE;
        break;
    case STX:
        packet_size = PACKET_1K_SIZE;
        break;
    case EOT:
        *length = 0;
        return 0;
    case CA:
        if (Receive_Byte(&ch, timeout) < 0) {
            return -2;
        }
        if (ch == CA) {
            *length = -1;
            return 0;
        } else {
            return -1;
        }
    case ABORT1:
    case ABORT2:
        return -2;
    default:
        Task_sleep(100);
        uart_consume();
        return -1;
    }

    *data = (uint8_t)ch;
    uint8_t *dptr = data+1;
    count = packet_size + PACKET_OVERHEAD-1;

    for (i=0; i<count; i++) {
        if (Receive_Byte(&ch, timeout) < 0) {
            return -1;
        }
        *dptr++ = (uint8_t)ch;;
    }

    if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff)) {
        *length = -2;
        return 0;
    }
    if (crc16(&data[PACKET_HEADER], packet_size + PACKET_TRAILER) != 0) {
        *length = -2;
        return 0;
    }

    *length = packet_size;
    uart_consume();     // YMODEM is half-duplex.  It expected that the receive buffer is empty at this point.
    return 0;
}


#if CONFIG_USE_LITTLEFS
static void lfs_cleanup(void)
{
    if(pFile != NULL) {
        lfs_c2000_fclose();
        pFile = NULL;
    }
    if(pLfs != NULL) {
        lfs_c2000_umount();
        pLfs = NULL;
    }
}
#endif

// Receive a file using the ymodem protocol.
//-----------------------------------------------------------------
int Ymodem_Receive (FILE *ffd, uint32_t maxsize, char* getname)
{
    uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
    uint8_t *file_ptr;
    char file_size[128];
    uint32_t i, file_len, write_len, session_done, file_done, packets_received, errors = 0;
    int32_t size = 0;
    int32_t packet_length = 0;
    file_len = 0;
    int32_t eof_cnt = 0;
    char * filename = NULL;

    for (session_done = 0, errors = 0; ;) {
        for (packets_received = 0, file_done = 0; ;) {
                switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT)) {
                    case 0:  // normal return
                        switch (packet_length) {
                            case -1:
                                // Abort by sender
                                send_ACK();
                                size = -1;
                                goto exit;
                            case -2:
                                // error
                                errors ++;
                                if (errors > 5) {
                                    send_CA();
                                    size = -2;
                                    goto exit;
                                }
                                send_NAK();
                                break;
                            case 0:
                                // End of transmission
                                eof_cnt++;
                                if (eof_cnt == 1) {
                                    send_NAK();
                                } else {
                                    send_ACKCRC16();
                                }
                                break;
                            default:
                                // ** Normal packet **
                                if (eof_cnt > 1) {
                                    send_ACK();
                                } else if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0x000000ff)) {
                                    errors ++;
                                    if (errors > 5) {
                                        send_CA();
                                        size = -3;
                                        goto exit;
                                    }
                                    send_NAK();
                                } else {
                                    if (packets_received == 0) {
                                        // ** First packet, Filename packet **
                                        if (packet_data[PACKET_HEADER] != 0) {
                                            errors = 0;
                                            // ** Filename packet has valid data
                                            filename = getname;
                                            if (getname) {
                                                for (i = 0, file_ptr = packet_data + PACKET_HEADER; ((*file_ptr != 0) && (i < FILE_NAME_LENGTH));) {
                                                    *getname = *file_ptr++;
                                                    getname++;
                                                }
                                                *getname = '\0';
                                            }
                                            for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < packet_length);) {
                                                file_ptr++;
                                            }
                                            for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);) {
                                                file_size[i++] = *file_ptr++;
                                            }
                                            file_size[i++] = '\0';
                                            if (strlen(file_size) > 0) {
                                                size = strtol(file_size, NULL, 10);
                                            } else {
                                                size = 0;
                                            }

                                            // Test the size of the file
                                            if ((size < 1) || (size > maxsize)) {
                                                // End session
                                                send_CA();
                                                if (size > maxsize) {
                                                    size = -9;
                                                } else {
                                                    size = -4;
                                                }
                                                goto exit;
                                            }
#if CONFIG_USE_LITTLEFS
                                            pLfs = lfs_c2000_mount();
                                            if(NULL == pLfs) {
                                                send_CA();
                                                size = -11;  // Filesystem failed to mount
                                                goto exit;
                                            }

                                            pFile = lfs_c2000_fopen(filename);
                                            if(NULL == pFile) {
                                                send_CA();
                                                size = -12;  // Failed to open file
                                                goto exit;
                                            }
#endif
                                            file_len = 0;
                                            send_ACKCRC16();
                                        } else {
                                            // Filename packet is empty, end session
                                            errors ++;
                                            if (errors > 5) {
                                                send_CA();
                                                size = -5;
                                                goto exit;
                                            }
                                            send_NAK();
                                        }
                                    } else {
                                        // ** Data packet **
                                        // Write received data to file
                                        if (file_len < size) {
                                            file_len += packet_length;  // total bytes received
                                            if (file_len > size) {
                                                write_len = packet_length - (file_len - size);
                                                file_len = size;
                                            } else {
                                                write_len = packet_length;
                                            }
#if CONFIG_USE_LITTLEFS
                                            int written_bytes = lfs_c2000_fwrite((char*)(packet_data + PACKET_HEADER), write_len);
#else
                                            //int written_bytes = fwrite((char*)(packet_data + PACKET_HEADER), 1, write_len, ffd);
                                            int written_bytes = write_len; // DUMMY while fwrite is not yet ported
#endif
                                            if (written_bytes != write_len) { //failed
                                                /* End session */
                                                send_CA();
                                                size = -6;
                                                goto exit;
                                            }
                                        }
                                        //success
                                        errors = 0;
                                        send_ACK();
                                    }
                                    packets_received++;
                                }
                        }
                        break;
                    case -2:  // user abort
                        send_CA();
                        size = -7;
                        goto exit;
                    default: // timeout
                        if (eof_cnt > 1) {
                            file_done = 1;
                        } else {
                            errors ++;
                            if (errors > MAX_ERRORS) {
                                send_CA();
                                size = -8;
                                goto exit;
                            }
                            send_CRC16();
                        }
                }
                if (file_done != 0) {
                    session_done = 1;
                    break;
                }
        }
        if (session_done != 0) {
            break;
        }
    }
exit:
#if CONFIG_USE_LITTLEFS
    lfs_cleanup();
#endif
    return size;
}


//------------------------------------------------------------------------------------
static void Ymodem_PrepareIntialPacket(uint8_t * data, char * fileName, uint32_t length)
{
    uint16_t tempCRC;

    memset(data, 0, PACKET_SIZE + PACKET_HEADER);
    // Make first three packet
    data[0] = SOH;
    data[1] = 0x00;
    data[2] = 0xff;

    // add filename
    size_t len = strlen(fileName);
    if(len > FILE_NAME_LENGTH) {
        len = FILE_NAME_LENGTH;
    }
    snprintf((char *)(data+PACKET_HEADER), len + 1, "%s", fileName);

    //add file site
    snprintf((char *)(data + PACKET_HEADER + len + 1), FILE_SIZE_LENGTH, "%ld", length);
    data[PACKET_HEADER + len + 1 + strlen((char *)(data + PACKET_HEADER + len + 1))] = ' ';

    // add crc
    tempCRC = crc16(&data[PACKET_HEADER], PACKET_SIZE);
    data[PACKET_SIZE + PACKET_HEADER] = (tempCRC >> 8) & 0xFF;
    data[PACKET_SIZE + PACKET_HEADER + 1] = tempCRC & 0xFF;
}


//-------------------------------------------------
static void Ymodem_PrepareLastPacket(uint8_t * data)
{
    uint16_t tempCRC;
  
    memset(data, 0, PACKET_SIZE + PACKET_HEADER);
    data[0] = SOH;
    data[1] = 0x00;
    data[2] = 0xff;
    tempCRC = crc16(&data[PACKET_HEADER], PACKET_SIZE);
    data[PACKET_SIZE + PACKET_HEADER] = (tempCRC >> 8) & 0xFF;
    data[PACKET_SIZE + PACKET_HEADER + 1] = tempCRC & 0xFF;
}


//-----------------------------------------------------------------------------------------
static void Ymodem_PreparePacket(uint8_t * data, uint8_t pktNo, uint32_t sizeBlk, FILE * ffd)
{
    uint16_t i, size;
    uint16_t tempCRC;

    data[0] = STX;
    data[1] = (pktNo & 0x000000ff);
    data[2] = (~(pktNo & 0x000000ff));

    size = sizeBlk < PACKET_1K_SIZE ? sizeBlk : PACKET_1K_SIZE;
    // Read block from file
    if (size > 0) {
#if CONFIG_USE_LITTLEFS
        size = lfs_file_read(pLfs, pFile, data + PACKET_HEADER, size);
#else
        //size = fread(data + PACKET_HEADER, 1, size, ffd);
        /* DUMMY */
        size = PACKET_1K_SIZE;
        for(uint16_t j = 0; j < PACKET_1K_SIZE; j++) {
            data[PACKET_HEADER + j] = j & 0xFF;
        }
#endif
    }

    if (size  < PACKET_1K_SIZE) {
        for (i = size + PACKET_HEADER; i < PACKET_1K_SIZE + PACKET_HEADER; i++) {
            data[i] = 0x00; // EOF (0x1A) or 0x00
        }
    }
    tempCRC = crc16(&data[PACKET_HEADER], PACKET_1K_SIZE);
    data[PACKET_1K_SIZE + PACKET_HEADER] = (tempCRC >> 8) & 0xFF;
    data[PACKET_1K_SIZE + PACKET_HEADER + 1] = tempCRC & 0xFF;
}


//-------------------------------------------------------------
static uint8_t Ymodem_WaitResponse(uint8_t ackchr, uint8_t tmo)
{
    uint8_t receivedC;
    uint32_t errors = 0;

    do {
        if (Receive_Byte(&receivedC, NAK_TIMEOUT) == 0) {
            if (receivedC == ackchr) {
                return 1;
            } else if (receivedC == CA) {
                send_CA();
                return 2; // CA received, Sender abort
            } else if (receivedC == NAK) {
                return 3;
            } else {
                return 4;
            }
        } else {
            errors++;
        }
    } while (errors < tmo);

    return 0;
}


static int32_t Send_Packet(uint8_t * packet, uint16_t size)
{
    int16_t remaining = size;
    uint16_t ret = 0;
    int32_t sent = 0;

    while(remaining > 0) {
        ret = UART_send(YMODEM_UART, (Char *)(packet + sent), remaining);
        sent += ret;
        remaining -= ret;
        if(remaining > 0) {
            Task_sleep(10);
        }
    }

    while(!UART_send_done(YMODEM_UART)) {
        Task_sleep(1);
    }

    return sent;
}


//------------------------------------------------------------------------
int Ymodem_Transmit (char * sendFileName, unsigned int sizeFile, FILE * ffd)
{
    uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
    uint16_t blkNumber;
    uint8_t receivedC;
    int32_t err;
    uint32_t size = 0;

    // Wait for response from receiver
    err = 0;
    do {
        send_CRC16();
    } while (Receive_Byte(&receivedC, NAK_TIMEOUT) < 0 && err++ < 45);

    if (err >= 45 || receivedC != CRC16) {
        send_CA();
        return -1;
    }

#if CONFIG_USE_LITTLEFS
    struct lfs_info fileInfo;
    pLfs = lfs_c2000_mount();
    if(NULL == pLfs) {
        send_CA();
        return -11;  // Filesystem failed to mount
    }

    pFile = lfs_c2000_fopen(sendFileName);
    if(NULL == pFile) {
        send_CA();
        lfs_c2000_umount();
        return -12;  // Failed to open file
    }

    if(LFS_ERR_OK != lfs_stat(pLfs, sendFileName, &fileInfo)) {
        send_CA();
        lfs_cleanup();
        return -13;   // Failed to get file Info
    }
    sizeFile = fileInfo.size;
#endif
    // === Prepare first block and send it =======================================
    /* When the receiving program receives this block and successfully
     * opened the output file, it shall acknowledge this block with an ACK
     * character and then proceed with a normal YMODEM file transfer
     * beginning with a "C" or NAK tranmsitted by the receiver.
     */
    Ymodem_PrepareIntialPacket(packet_data, sendFileName, sizeFile);
    do {
        // Send Packet
        Send_Packet(packet_data, PACKET_SIZE + PACKET_OVERHEAD);
        // Wait for Ack
        err = Ymodem_WaitResponse(ACK, 10);
        if (err == 0 || err == 4) {
            send_CA();
#if CONFIG_USE_LITTLEFS
            lfs_cleanup();
#endif
            return -2;                  // timeout or wrong response
        } else if (err == 2) {
#if CONFIG_USE_LITTLEFS
            lfs_cleanup();
#endif
            return 98; // abort
        }
    } while (err != 1);

    // After initial block the receiver sends 'C' after ACK
    if (Ymodem_WaitResponse(CRC16, 10) != 1) {
        send_CA();
#if CONFIG_USE_LITTLEFS
        lfs_cleanup();
#endif
        return -3;
    }

    // === Send file blocks ======================================================
    size = sizeFile;
    blkNumber = 0x01;

    // Resend packet if NAK  for a count of 10 else end of communication
    while (size) {
        // Prepare and send next packet
        Ymodem_PreparePacket(packet_data, blkNumber, size, ffd);
        do {
            Send_Packet(packet_data, PACKET_1K_SIZE + PACKET_OVERHEAD);

            // Wait for Ack
            err = Ymodem_WaitResponse(ACK, 10);
            if (err == 1) {
                blkNumber++;
                if (size > PACKET_1K_SIZE) {
                    size -= PACKET_1K_SIZE; // Next packet
                } else {
                    size = 0; // Last packet sent
                }
            } else if (err == 0 || err == 4) {
                send_CA();
#if CONFIG_USE_LITTLEFS
                lfs_cleanup();
#endif
                return -4;                  // timeout or wrong response
            } else if (err == 2) {
#if CONFIG_USE_LITTLEFS
                lfs_cleanup();
#endif
                return -5; // abort
            }
        } while(err != 1);
    }

    // === Send EOT ==============================================================
    uint16_t timeout;
    timeout = 100;
    Send_Byte(EOT); // Send (EOT)
    while(!UART_send_done(YMODEM_UART)) {
        Task_sleep(1);
        timeout--;
        if(timeout == 0) {
            break;
        }
    }
    // Wait for Ack
    do {
        // Wait for Ack
        err = Ymodem_WaitResponse(ACK, 10);
        if (err == 3) {   // NAK
            timeout = 100;
            Send_Byte(EOT); // Send (EOT)
            while(!UART_send_done(YMODEM_UART)) {
                Task_sleep(1);
                timeout--;
                if(timeout == 0) {
                    break;
                }
            }
        } else if (err == 0 || err == 4) {
            send_CA();
#if CONFIG_USE_LITTLEFS
            lfs_cleanup();
#endif
            return -6;                  // timeout or wrong response
        } else if (err == 2) {
#if CONFIG_USE_LITTLEFS
            lfs_cleanup();
#endif
            return -7; // abort
        }
    } while (err != 1);

    // === Receiver requests next file, prepare and send last packet =============
    if (Ymodem_WaitResponse(CRC16, 10) != 1) {
        send_CA();
#if CONFIG_USE_LITTLEFS
        lfs_cleanup();
#endif
        return -8;
    }

    Ymodem_PrepareLastPacket(packet_data);
    do {
        // Send Packet
        Send_Packet(packet_data, PACKET_SIZE + PACKET_OVERHEAD);

        // Wait for Ack
        err = Ymodem_WaitResponse(ACK, 10);
        if (err == 0 || err == 4) {
            send_CA();
#if CONFIG_USE_LITTLEFS
            lfs_cleanup();
#endif
            return -9;  // timeout or wrong response
        } else if (err == 2) {
#if CONFIG_USE_LITTLEFS
            lfs_cleanup();
#endif
            return -10; // abort
        }
    } while (err != 1);

#if CONFIG_USE_LITTLEFS
    lfs_cleanup();
#endif
    return 0; // file transmitted successfully
}

#endif /* CONFIG_USE_YMODEM */


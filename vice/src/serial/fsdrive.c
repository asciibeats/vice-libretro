/*
 * fsdrive.c - Filesystem based serial emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "attach.h"
#include "fsdrive.h"
#include "log.h"
#include "serial.h"
#include "types.h"

/*
 * This code named "fsdrive" is not to be confused with "fsdevice".
 *
 * Together with serial/serial-iec-device.c it does much the same thing as
 * parallel/parallel-trap.c, but in a somewhat different way, and it has
 * serial/serial-iec-bus.c wedged in between...
 *
 * serial/serial-trap.c corresponds somewhat with serial-iec-bus.c when
 * kernel traps are in use.
 */
/* #define FSDRIVE_DEBUG */
#ifdef FSDRIVE_DEBUG
#define DBG(_x_)        log_debug _x_
#else
#define DBG(_x_)
#endif

#define SERIAL_NAMELENGTH 255

static log_t fsdrive_log = LOG_ERR;

static uint8_t SerialBuffer[SERIAL_NAMELENGTH + 1];
static int SerialPtr;

/*
   On a real system an opened channel is affected only after having
   received and parsed the complete next open command.
   This patch tries to emulate this behavior and allows to keep the
   status channel continuously open while opening and closing other
   files on the same device.

   FIXME: test for regressions and either revert to, or remove old code
*/
#define DELAYEDCLOSE

/* Handle Serial Bus Commands under Attention.  */
static uint8_t serialcommand(unsigned int device, uint8_t secondary)
{
    serial_t *p;
    void *vdrive;
    int channel;
    int i;
    uint8_t st = 0;

    /*
     * which device ?
     */
    p = serial_device_get(device & 0x0f);
    channel = secondary & 0x0f;

    if ((device & 0x0f) >= 8) {
        /* TODO serial devices only have a single drive */
        vdrive = (void *)file_system_get_vdrive(device & 0x0f, 0);
    } else {
        vdrive = NULL;
    }

    /* if command on a channel, reset output buffer... */
    if ((secondary & 0xf0) != 0x60) {
        p->nextok[channel] = 0;
    }
    switch (secondary & 0xf0) {
        case 0x20:
        case 0x30:
            DBG(("2x/3x: LISTEN, DEV = %d (no call to driver)", secondary & 0x1F));
            break;
        case 0x40:
        case 0x50:
            DBG(("4x/5x: TALK, DEV = %d (no call to driver)", secondary & 0x1F));
            break;
        /*
         * Open Channel
         */
        case 0x60:
            DBG(("6x: OPEN CHANNEL, SA = %d (isopen[channel]=%d, 1 calls ->openf) SerialPtr=%d", secondary & 0x0F, p->isopen[channel], SerialPtr));
            if (p->isopen[channel] == ISOPEN_AWAITING_NAME) {
                p->isopen[channel] = ISOPEN_OPEN;
                st = (uint8_t)((*(p->openf))(vdrive, NULL, 0, channel, NULL));

                for (i = 0; i < SerialPtr; i++) {
                    (*(p->putf))(vdrive, ((uint8_t)(SerialBuffer[i])), channel);
                    DBG(("SerialBuffer: %c", SerialBuffer[i]));
                }
                SerialPtr = 0;
            }
            if (p->flushf) {
                (*(p->flushf))(vdrive, channel);
            }
            break;

        /*
         * Close File
         */
        case 0xE0:
            DBG(("Ex: CLOSE FILE, SA = %d", secondary & 0x0F));
            p->isopen[channel] = ISOPEN_CLOSED;
            st = (uint8_t)((*(p->closef))(vdrive, channel));
            break;

        /*
         * Open File
         */
        case 0xF0:
            DBG(("Fx: OPEN FILE, SA = %d", secondary & 0x0F));
            if (p->isopen[channel] != ISOPEN_CLOSED) {
#ifndef DELAYEDCLOSE
                if (p->isopen[channel] == ISOPEN_OPEN) {
                    log_warning(fsdrive_log, "Bogus close?");
                    (*(p->closef))(vdrive, channel);
                }
                p->isopen[channel] = ISOPEN_OPEN;
                SerialBuffer[SerialPtr] = 0;
                st = (uint8_t)((*(p->openf))(vdrive, SerialBuffer, SerialPtr,
                                          channel, NULL));
                SerialPtr = 0;

                if (st) {
                    p->isopen[channel] = ISOPEN_CLOSED;
                    (*(p->closef))(vdrive, channel);

                    log_error(fsdrive_log, "Cannot open file. Status $%02x.", st);
                }
#else
                if (SerialPtr != 0 || channel == 0x0f) {
                    (*(p->closef))(vdrive, channel);
                    p->isopen[channel] = ISOPEN_OPEN;
                    SerialBuffer[SerialPtr] = 0;
                    st = (uint8_t)((*(p->openf))(vdrive, SerialBuffer, SerialPtr,
                                              channel, NULL));
                    SerialPtr = 0;
                    if (st) {
                        p->isopen[channel] = ISOPEN_CLOSED;
                        (*(p->closef))(vdrive, channel);

                        log_error(fsdrive_log, "Cannot open file. Status $%02x.", st);
                    }
                }
#endif
            }
            if (p->flushf) {
                (*(p->flushf))(vdrive, channel);
            }
            break;

        default:
            log_error(fsdrive_log, "Unknown command %02X.", secondary & 0xff);
    }

    return st;
}

/* ------------------------------------------------------------------------- */

void fsdrive_open(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    serial_t *p;
#ifndef DELAYEDCLOSE
    void *vdrive;
#endif

    p = serial_device_get(device & 0x0f);
    DBG(("fsdrive_open %u,%d", device & 0xF, secondary & 0xF));
#ifndef DELAYEDCLOSE
    if (p->isopen[secondary & 0x0f] == ISOPEN_OPEN) {
        if ((device & 0x0f) >= 8) {
            vdrive = (void *)file_system_get_vdrive(device & 0x0f);
        } else {
            vdrive = NULL;
        }
        (*(p->closef))(vdrive, secondary & 0x0f);
    }
#endif
    p->isopen[secondary & 0x0f] = ISOPEN_AWAITING_NAME;
}

void fsdrive_close(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    uint8_t st;

    st = serialcommand(device, secondary);
    st_func(st);
}

void fsdrive_listentalk(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    uint8_t st;
    serial_t *p;
    void *vdrive;

    st = serialcommand(device, secondary);
    st_func(st);

    p = serial_device_get(device & 0x0f);
    if (p->listenf) {
        /* send listen/talk to emulated devices for flushing of
           REL file write buffer. */
        if ((device & 0x0f) >= 8) {
            /* single drive only */
            vdrive = (void *)file_system_get_vdrive(device & 0x0f, 0);
            (*(p->listenf))(vdrive, secondary & 0x0f);
        }
    }
}

void fsdrive_unlisten(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    uint8_t st;
    serial_t *p;
    void *vdrive;

    p = serial_device_get(device & 0x0f);
    if ((secondary & 0xf0) == 0xf0
        || (secondary & 0x0f) == 0x0f) {
        st = serialcommand(device, secondary);
        st_func(st);
        /* Flush serial read ahead buffer too.  */
        p->nextok[secondary & 0x0f] = 0;
    } else if (p->listenf) {
        /* send unlisten to emulated devices for flushing of
           REL file write buffer. */
        if ((device & 0x0f) >= 8) {
            /* single drive only */
            vdrive = (void *)file_system_get_vdrive(device & 0x0f, 0);
            (*(p->listenf))(vdrive, secondary & 0x0f);
        }
    }
}

void fsdrive_untalk(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
}

void fsdrive_write(unsigned int device, uint8_t secondary, uint8_t data, void (*st_func)(uint8_t))
{
    uint8_t st;
    serial_t *p;
    void *vdrive;

    p = serial_device_get(device & 0x0f);

    if ((device & 0x0f) >= 8) {
        vdrive = (void *)file_system_get_vdrive(device & 0x0f, 0);
    } else {
        vdrive = NULL;
    }

    if (p->inuse) {
        if (p->isopen[secondary & 0x0f] == ISOPEN_AWAITING_NAME) {
            /* Store name here */
            if (SerialPtr < SERIAL_NAMELENGTH) {
                DBG(("SerialBuffer[%d] = '%c'", SerialPtr, data));
                SerialBuffer[SerialPtr++] = data;
            }
        } else {
            /* Send to device */
            st = (*(p->putf))(vdrive, data, (int)(secondary & 0x0f));
            st_func(st);
        }
    } else {                    /* Not present */
        st_func(0x83);
    }
}

uint8_t fsdrive_read(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    int st = 0, secadr = secondary & 0x0f;
    uint8_t data;
    serial_t *p;
    void *vdrive;

    p = serial_device_get(device & 0x0f);

    if ((device & 0x0f) >= 8) {
        vdrive = (void *)file_system_get_vdrive(device & 0x0f, 0);
    } else {
        vdrive = NULL;
    }

#if 0
    /* Get next byte if necessary.  */
    if (!(p->nextok[secadr]))
#endif
    st = (*(p->getf))(vdrive, &(p->nextbyte[secadr]), secadr);

    /* Move byte from buffer to output.  */
    data = p->nextbyte[secadr];
    p->nextok[secadr] = 0;
#if 0
    /* Fill buffer again.  */
    if (!st) {
        st = (*(p->getf))(vdrive, &(p->nextbyte[secadr]), secadr);
        if (!st) {
            p->nextok[secadr] = 1;
        }
    }
#endif
    st_func((uint8_t)st);

    return data;
}

void fsdrive_reset(void)
{
    unsigned int i, j;
    serial_t *p;
    void *vdrive;

    for (i = 0; i < SERIAL_MAXDEVICES; i++) {
        p = serial_device_get(i);
        if (p->inuse) {
            for (j = 0; j < 16; j++) {
                if (p->isopen[j] != ISOPEN_CLOSED) {
                    vdrive = (void *)file_system_get_vdrive(i, 0);
                    p->isopen[j] = ISOPEN_CLOSED;
                    (*(p->closef))(vdrive, j);
                }
            }
        }
    }
}

void fsdrive_init(void)
{
    fsdrive_log = log_open("FSDrive");
}

/*
 * archdep.h - Miscellaneous system-specific stuff.
 *
 * Written by
 *  Akop Karapetyan <dev@psp.akop.org>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef _ARCHDEP_H
#define _ARCHDEP_H

#include "sysconfig.h"

extern char retro_system_data_directory[512];
#define LIBDIR retro_system_data_directory

extern char retro_save_directory[512];
#define SAVEDIR retro_save_directory

#define ARCHDEP_PRINTER_DEFAULT "vice_printer.txt"
#if 0
#define RETRO_DEBUG
#endif

#define VICE_ARCHAPI_PRIVATE_API
#include "archapi.h"
#undef VICE_ARCHAPI_PRIVATE_API

/* Default sound output mode */
#define ARCHDEP_SOUND_OUTPUT_MODE SOUND_OUTPUT_STEREO

#if defined(__WIN32__) 
/* Filesystem dependant operators.  */
#define FSDEVICE_DEFAULT_DIR "."
#define FSDEV_DIR_SEP_STR    "\\"
#define FSDEV_DIR_SEP_CHR    '\\'
#define FSDEV_EXT_SEP_STR    "."
#define FSDEV_EXT_SEP_CHR    '.'

/* Path separator.  */
#define ARCHDEP_FINDPATH_SEPARATOR_CHAR   ';'
#define ARCHDEP_FINDPATH_SEPARATOR_STRING ";"


/* Modes for fopen().  */
#define MODE_READ              "rb"
#define MODE_READ_TEXT         "rb"
#define MODE_READ_WRITE        "rb+"
#define MODE_WRITE             "wb"
#define MODE_WRITE_TEXT        "wb"
#define MODE_APPEND            "a"
#define MODE_APPEND_READ_WRITE "a+"

/* Printer default devices.  */
#define ARCHDEP_PRINTER_DEFAULT_DEV1 "viceprnt.out"
#define ARCHDEP_PRINTER_DEFAULT_DEV2 "LPT1:"
#define ARCHDEP_PRINTER_DEFAULT_DEV3 "LPT2:"

/* Default RS232 devices.  */
#define ARCHDEP_RS232_DEV1 "10.0.0.1:25232"
#define ARCHDEP_RS232_DEV2 "10.0.0.1:25232"
#define ARCHDEP_RS232_DEV3 "10.0.0.1:25232"
#define ARCHDEP_RS232_DEV4 "10.0.0.1:25232"

/* Default location of raw disk images.  */
#define ARCHDEP_RAWDRIVE_DEFAULT "A:"

/* Access types */
#define ARCHDEP_R_OK 4
#define ARCHDEP_W_OK 2
#define ARCHDEP_X_OK 1
#define ARCHDEP_F_OK 0

/* Standard line delimiter.  */
#define ARCHDEP_LINE_DELIMITER "\r\n"

/* Ethernet default device */
#define ARCHDEP_ETHERNET_DEFAULT_DEVICE ""
#define archdep_signals_init(x)
#define archdep_signals_pipe_set()
#define archdep_signals_pipe_unset()

#else

/* Filesystem dependant operators.  */
#define FSDEVICE_DEFAULT_DIR   "."
#define FSDEV_DIR_SEP_STR      "/"
#define FSDEV_DIR_SEP_CHR      '/'
#define FSDEV_EXT_SEP_STR      "."
#define FSDEV_EXT_SEP_CHR      '.'

/* Path separator.  */
#define ARCHDEP_FINDPATH_SEPARATOR_CHAR         ':'
#define ARCHDEP_FINDPATH_SEPARATOR_STRING       ":"


/* Modes for fopen().  */
#define MODE_READ              "r"
#define MODE_READ_TEXT         "r"
#define MODE_READ_WRITE        "r+"
#define MODE_WRITE             "w"
#define MODE_WRITE_TEXT        "w"
#define MODE_APPEND            "w+"
#define MODE_APPEND_READ_WRITE "a+"

/* Printer default devices.  */
#define ARCHDEP_PRINTER_DEFAULT_DEV1 "print.dump"
#define ARCHDEP_PRINTER_DEFAULT_DEV2 "|lpr"
#define ARCHDEP_PRINTER_DEFAULT_DEV3 "|petlp -F PS|lpr"

/* Default RS232 devices.  */
#define ARCHDEP_RS232_DEV1 "/dev/ttyS0"
#define ARCHDEP_RS232_DEV2 "/dev/ttyS1"
#define ARCHDEP_RS232_DEV3 "rs232.dump"
#define ARCHDEP_RS232_DEV4 "|lpr"

/* Default location of raw disk images.  */
#define ARCHDEP_RAWDRIVE_DEFAULT "/dev/fd0"

/* Access types */
#define ARCHDEP_R_OK R_OK
#define ARCHDEP_W_OK W_OK
#define ARCHDEP_X_OK X_OK
#define ARCHDEP_F_OK F_OK

/* Standard line delimiter.  */
#define ARCHDEP_LINE_DELIMITER "\n"

/* Ethernet default device */
#define ARCHDEP_ETHERNET_DEFAULT_DEVICE ""
#define archdep_signals_init(x)
#define archdep_signals_pipe_set()
#define archdep_signals_pipe_unset()

#endif

/* Video chip scaling.  */
#define ARCHDEP_VICII_DSIZE   1
#define ARCHDEP_VICII_DSCAN   0
#define ARCHDEP_VICII_HWSCALE 0
#define ARCHDEP_VDC_DSIZE     1
#define ARCHDEP_VDC_DSCAN     0
#define ARCHDEP_VDC_HWSCALE   0
#define ARCHDEP_VIC_DSIZE     1
#define ARCHDEP_VIC_DSCAN     0
#define ARCHDEP_VIC_HWSCALE   0
#define ARCHDEP_CRTC_DSIZE    1
#define ARCHDEP_CRTC_DSCAN    0
#define ARCHDEP_CRTC_HWSCALE  0
#define ARCHDEP_TED_DSIZE     1
#define ARCHDEP_TED_DSCAN     0
#define ARCHDEP_TED_HWSCALE   0

/* Video chip double buffering.  */
#define ARCHDEP_VICII_DBUF 0
#define ARCHDEP_VDC_DBUF   0
#define ARCHDEP_VIC_DBUF   0
#define ARCHDEP_CRTC_DBUF  0
#define ARCHDEP_TED_DBUF   0

#define ARCHDEP_SOCKET_ERROR errno

/* No key symcode.  */
#define ARCHDEP_KEYBOARD_SYM_NONE 0
/* Keyword to use for a static prototype */
#define STATIC_PROTOTYPE static

/* Define the default system directory (where the ROMs are).  */
#ifndef __LIBRETRO__
#ifdef __NetBSD__
#define LIBDIR          PREFIX "/share/vice"
#else
#define LIBDIR          PREFIX "./vice"
#endif
#endif /*__LIBRETRO__ */

#if defined(__FreeBSD__) || defined(__NetBSD__)
#define DOCDIR          PREFIX "/share/doc/vice"
#else
#define DOCDIR          LIBDIR "./doc"
#endif

#ifdef __PS3__
#define F_OK	0
#define W_OK	0
#define R_OK	0
#define X_OK	0
#define chdir(...) 0
#define isatty(...) 0
#define currentpath "/dev_hdd0/game/RETROARCH/USRDIR"
#define tmppath "/dev_hdd0/game/RETROARCH/USRDIR/savefiles"
#define getwd(buffer) (strcpy(buffer, currentpath)) ? (buffer) : (NULL)
#define tmpnam(...) (tmppath)
#define getenv(...) (NULL)
#define VICEUSERDIR     "/dev_hdd0/game/RETROARCH/USRDIR/system/vice"
int access(const char *fpath, int mode);
#else
#define VICEUSERDIR     ".vice"
#endif

#endif /*_ARCHDEP_H */

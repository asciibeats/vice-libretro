#ifndef LIBRETRO_CORE_H
#define LIBRETRO_CORE_H 1

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>

#include "vkbd.h"
#include "retroglue.h"
#include "retro_files.h"
#include "retro_strings.h"
#include "retro_disk_control.h"
#include "string/stdstring.h"
#include "file/file_path.h"
#include "compat/strcasestr.h"

#define MATRIX(a,b) (((a) << 3) | (b))
#define RGB565(r, g, b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))
#define RGB888(r, g, b) (((r * 255 / 31) << 16) | ((g * 255 / 31) << 8) | (b * 255 / 31))
#define ARGB888(a, r, g, b) ((a << 24) | (r << 16) | (g << 8) | b)

/* Log */
#if defined(__ANDROID__) || defined(ANDROID)
#include <android/log.h>
#define LOG_TAG "RetroArch.vice"
#endif

/* Types */
#define UINT16 uint16_t
#define UINT32 uint32_t
typedef uint32_t uint32;
typedef uint8_t uint8;

/* Screen */
#if defined(__X128__)
#define WINDOW_WIDTH  856
#define WINDOW_HEIGHT 312
#elif defined(__XPET__)
#define WINDOW_WIDTH  704
#define WINDOW_HEIGHT 288
#elif defined(__XCBM2__)
#define WINDOW_WIDTH  704
#define WINDOW_HEIGHT 366
#elif defined(__XVIC__)
#define WINDOW_WIDTH  448
#define WINDOW_HEIGHT 288
#else
#define WINDOW_WIDTH  384
#define WINDOW_HEIGHT 288
#endif
#define RETRO_BMP_SIZE (WINDOW_WIDTH * WINDOW_HEIGHT * 2)
extern unsigned short int retro_bmp[RETRO_BMP_SIZE];
extern unsigned int pix_bytes;

#define MANUAL_CROP_OPTIONS \
   { \
      { "0", NULL }, \
      { "1", NULL }, { "2", NULL }, { "3", NULL }, { "4", NULL }, { "5", NULL }, { "6", NULL }, { "7", NULL }, { "8", NULL }, { "9", NULL }, { "10", NULL }, \
      { "11", NULL }, { "12", NULL }, { "13", NULL }, { "14", NULL }, { "15", NULL }, { "16", NULL }, { "17", NULL }, { "18", NULL }, { "19", NULL }, { "20", NULL }, \
      { "21", NULL }, { "22", NULL }, { "23", NULL }, { "24", NULL }, { "25", NULL }, { "26", NULL }, { "27", NULL }, { "28", NULL }, { "29", NULL }, { "30", NULL }, \
      { "31", NULL }, { "32", NULL }, { "33", NULL }, { "34", NULL }, { "35", NULL }, { "36", NULL }, { "37", NULL }, { "38", NULL }, { "39", NULL }, { "40", NULL }, \
      { "41", NULL }, { "42", NULL }, { "43", NULL }, { "44", NULL }, { "45", NULL }, { "46", NULL }, { "47", NULL }, { "48", NULL }, { "49", NULL }, { "50", NULL }, \
      { "51", NULL }, { "52", NULL }, { "53", NULL }, { "54", NULL }, { "55", NULL }, { "56", NULL }, { "57", NULL }, { "58", NULL }, { "59", NULL }, { "60", NULL }, \
      { NULL, NULL }, \
   }

/* VKBD */
#define VKBDX 11
#define VKBDY 7

#if 0
#define POINTER_DEBUG
#endif
#ifdef POINTER_DEBUG
extern int pointer_x;
extern int pointer_y;
#endif

extern int vkey_pos_x;
extern int vkey_pos_y;
extern int vkey_pressed;
extern int vkey_sticky;
extern int vkey_sticky1;
extern int vkey_sticky2;

extern int vkbd_x_min;
extern int vkbd_x_max;
extern int vkbd_y_min;
extern int vkbd_y_max;

/* Statusbar */
#define STATUSBAR_BOTTOM  0x01
#define STATUSBAR_TOP     0x02
#define STATUSBAR_BASIC   0x04
#define STATUSBAR_MINIMAL 0x08

/* Variables */
extern int cpuloop;
extern int retroXS;
extern int retroYS;
extern int retroXS_offset;
extern int retroYS_offset;
extern int retroH;
extern int retroW;
extern unsigned int zoomed_width;
extern unsigned int zoomed_height;
extern unsigned int zoomed_XS_offset;
extern unsigned int zoomed_YS_offset;
extern int imagename_timer;
extern unsigned int opt_statusbar;
extern unsigned int cur_port;
extern unsigned int retro_region;

enum {
   RUNSTATE_FIRST_START = 0,
   RUNSTATE_LOADED_CONTENT,
   RUNSTATE_RUNNING,
};

/* Functions */
extern void maincpu_mainloop_retro(void);
extern long GetTicks(void);
extern void retro_audio_render(signed short int *sound_buffer, int sndbufsize);

/* Core options */
struct libretro_core_options {
   int Model;
   int UserportJoyType;
   int AutostartWarp;
   int AttachDevice8Readonly;
   int DriveTrueEmulation;
   int DriveSoundEmulation;
   int AudioLeak;
   int SoundSampleRate;
   int SidEngine;
   int SidModel;
   int SidExtra;
   int SidResidSampling;
   int SidResidPassband;
   int SidResidGain;
   int SidResidFilterBias;
   int SidResid8580FilterBias;
   char ExternalPalette[RETRO_PATH_MAX];
   int ColorGamma;
   int ColorTint;
   int ColorSaturation;
   int ColorContrast;
   int ColorBrightness;
#if defined(__X128__)
   int C128ColumnKey;
   int Go64Mode;
#elif defined(__XVIC__)
   int VIC20Memory;
#endif
};

extern struct libretro_core_options core_opt;
#endif

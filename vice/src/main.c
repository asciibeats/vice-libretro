/*
 * main.c - VICE main startup entry.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Vesa-Matti Puro <vmp@lut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

/* #define DEBUG_MAIN */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef USE_VICE_THREAD
#include <pthread.h>
#endif

#include "archdep.h"
#include "cmdline.h"
#include "console.h"
#include "debug.h"
#include "drive.h"
#include "fullscreen.h"
#include "gfxoutput.h"
#include "info.h"
#include "init.h"
#include "initcmdline.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "main.h"
#include "mainlock.h"
#include "resources.h"
#include "sysfile.h"
#include "tick.h"
#include "types.h"
#include "uiapi.h"
#include "version.h"
#include "video.h"
#include "vsyncapi.h"

#ifdef __LIBRETRO__
#include "libretro-core.h"
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
#define LOG_HR "--------------------------------------------------------------------------------"
#endif

#ifdef USE_SVN_REVISION
#include "svnversion.h"
#endif

#ifdef DEBUG_MAIN
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

int console_mode = 0;
int video_disabled_mode = 0;

void main_loop_forever(void);

#ifdef USE_VICE_THREAD
void *vice_thread_main(void *);
static pthread_t vice_thread;
#endif


/** \brief  Size of buffer used to write core team members' names to log/stdout
 *
 * 79 characters + 1 byte for '\0'. Assuming a terminal width of 80 characters,
 * we can only use 79 when calling log_message() since that function adds a
 * newline to its ouput.
 */
#define TERM_TMP_SIZE  80

/* ------------------------------------------------------------------------- */

/* This is the main program entry point.  Call this from `main()'.  */
int main_program(int argc, char **argv)
{
    int i, n;
    const char *program_name;
    int ishelp = 0;
    int loadconfig = 1;
    char term_tmp[TERM_TMP_SIZE];
    size_t name_len;

    lib_init();

    /* Check for some options at the beginning of the commandline before 
       initializing the user interface or loading the config file.
       -default => use default config, do not load any config
       -config  => use specified configuration file
       -console => no user interface
    */
    DBG(("main:early cmdline(argc:%d)\n", argc));
    for (i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-console")) || (!strcmp(argv[i], "--console"))) {
            console_mode = 1;
            video_disabled_mode = 1;
        } else
        if ((!strcmp(argv[i], "-config")) || (!strcmp(argv[i], "--config"))) {
            if ((i + 1) < argc) {
                vice_config_file = lib_strdup(argv[++i]);
                loadconfig = 1;
            }
        } else if (!strcmp(argv[i], "-default")) {
            loadconfig = 0;
        } else {
            break;
        }
    }
    /* remove the already handled items from the commandline, else they will
       get parsed again later, which causes surprising effects. */
    for (n = 1; i < argc; n++, i++) {
        argv[n] = argv[i];
    }
    argv[n] = NULL;
    argc = n;

    /* help is also special, but we want it NOT to be ignored by the main
       commandline handler */
    for (i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-help")) ||
                   (!strcmp(argv[i], "--help")) ||
                   (!strcmp(argv[i], "-h")) ||
                   (!strcmp(argv[i], "-?"))) {
            ishelp = 1;
        }
    }

    DBG(("main:archdep_init(argc:%d)\n", argc));
    if (archdep_init(&argc, argv) != 0) {
        archdep_startup_log_error("archdep_init failed.\n");
        return -1;
    }

    tick_init();
    maincpu_early_init();
    machine_setup_context();
    drive_setup_context();
    machine_early_init();

    /* Initialize system file locator.  */
    sysfile_init(machine_name);

    gfxoutput_early_init(ishelp);
    if ((init_resources() < 0) || (init_cmdline_options() < 0)) {
        return -1;
    }

    /* Set factory defaults.  */
    if (resources_set_defaults() < 0) {
        archdep_startup_log_error("Cannot set defaults.\n");
        return -1;
    }

    /* Initialize the user interface.  `ui_init()' might need to handle the
       command line somehow, so we call it before parsing the options.
       (e.g. under X11, the `-display' option is handled independently).  */
    DBG(("main:ui_init(argc:%d)\n", argc));
    if (!console_mode && ui_init(&argc, argv) < 0) {
        archdep_startup_log_error("Cannot initialize the UI.\n");
        return -1;
    }

    if ((!ishelp) && (loadconfig)) {
        /* Load the user's default configuration file.  */
        if (resources_reset_and_load(NULL) < 0) {
            /* The resource file might contain errors, and thus certain
            resources might have been initialized anyway.  */
            if (resources_set_defaults() < 0) {
                archdep_startup_log_error("Cannot set defaults.\n");
                return -1;
            }
        }
    }

    if (log_init() < 0) {
        const char *logfile = NULL;

        /* assuming LogFileName exists */
        resources_get_string("LogFileName", &logfile);

        if (logfile != NULL && *logfile != '\0') {
            archdep_startup_log_error(
                    "Cannot start logging system, failed to open '%s' for writing",
                    logfile);
        } else {
            archdep_startup_log_error("Cannot startup logging system.\n");
        }
    }

    DBG(("main:initcmdline_check_args(argc:%d)\n", argc));
    if (initcmdline_check_args(argc, argv) < 0) {
        return -1;
    }

    program_name = archdep_program_name();

    /* VICE boot sequence.  */
#ifdef __LIBRETRO__
    log_message(LOG_DEFAULT, LOG_HR);
    log_message(LOG_DEFAULT, "VICE %s (%s) %s%s, %s %s", program_name, machine_name, VERSION, GIT_VERSION, __DATE__, __TIME__);
    log_message(LOG_DEFAULT, LOG_HR);
#else /* __LIBRETRO __ */
    log_message(LOG_DEFAULT, " ");
#ifdef USE_SVN_REVISION
    log_message(LOG_DEFAULT, "*** VICE Version %s, rev %s ***", VERSION, VICE_SVN_REV_STRING);
#else
    log_message(LOG_DEFAULT, "*** VICE Version %s ***", VERSION);
#endif
    log_message(LOG_DEFAULT, " ");
    if (machine_class == VICE_MACHINE_VSID) {
        log_message(LOG_DEFAULT, "Welcome to %s, the free portable SID Player.",
                    program_name);
    } else {
        log_message(LOG_DEFAULT, "Welcome to %s, the free portable %s Emulator.",
                    program_name, machine_name);
    }
    log_message(LOG_DEFAULT, " ");

    log_message(LOG_DEFAULT, "Current VICE team members:");
    n = 0; *term_tmp = 0;
    for (i = 0; core_team[i].name != NULL; i++) {
        name_len = strlen(core_team[i].name);
        /* XXX: reject names that will never fit, for now */
        if ((int)name_len + 3 > TERM_TMP_SIZE) {
            log_warning(LOG_DEFAULT, "%s:%d: name '%s' too large for buffer",
                    __FILE__, __LINE__, core_team[i].name);
            break;  /* this will still write out whatever was in the buffer */
        }

        if (n + (int)name_len + 3 > TERM_TMP_SIZE) {    /* +3 for ", \0" */
            log_message(LOG_DEFAULT, "%s", term_tmp);
            strcpy(term_tmp, core_team[i].name);
            n = (int)name_len;
        } else {
            strcat(term_tmp, core_team[i].name);
            n += (int)name_len;
        }
        if (core_team[i + 1].name == NULL) {
            strcat(term_tmp, ".");
        } else {
            strcat(term_tmp, ", ");
            n += 2;
        }
    }
    log_message(LOG_DEFAULT, "%s", term_tmp);

    log_message(LOG_DEFAULT, " ");
    log_message(LOG_DEFAULT, "This is free software with ABSOLUTELY NO WARRANTY.");
    log_message(LOG_DEFAULT, "See the \"About VICE\" command for more info.");
    log_message(LOG_DEFAULT, " ");
#endif /* __LIBRETRO__ */

    /* lib_free(program_name); */

    /* Complete the GUI initialization (after loading the resources and
       parsing the command-line) if necessary.  */
    if (!console_mode && ui_init_finish() < 0) {
        return -1;
    }

    if (!console_mode && video_init() < 0) {
        return -1;
    }

    if (initcmdline_check_psid() < 0) {
        return -1;
    }
    
    if (init_main() < 0) {
        return -1;
    }
    
    initcmdline_check_attach();

#ifdef USE_VICE_THREAD

    if (pthread_create(&vice_thread, NULL, vice_thread_main, NULL)) {
        log_error(LOG_DEFAULT, "Fatal: failed to launch main thread");
        return 1;
    }

#else /* #ifdef USE_VICE_THREAD */

    main_loop_forever();

#endif /* #ifdef USE_VICE_THREAD */

    return 0;
}

void main_loop_forever(void)
{
    log_message(LOG_DEFAULT, "Main CPU: starting at ($FFFC).");

    /* This doesn't return. The thread will directly exit when requested. */
    maincpu_mainloop();

#ifndef __LIBRETRO__
    log_error(LOG_DEFAULT, "perkele! (THREAD)");
#endif
}

#ifdef USE_VICE_THREAD

void vice_thread_shutdown(void)
{
    if (!vice_thread) {
        /* We're exiting early in program life, such as when invoked with -help */
        return;
    }

    if (pthread_equal(pthread_self(), vice_thread)) {
        printf("FIXME! VICE thread is trying to shut itself down directly, this needs to be called from the ui thread for a correct shutdown!\n");
        mainlock_initiate_shutdown();
        return;
    }

    mainlock_obtain();
    mainlock_initiate_shutdown();
    mainlock_release();

    pthread_join(vice_thread, NULL);

    log_message(LOG_DEFAULT, "VICE thread has been joined.");
}

void *vice_thread_main(void *unused)
{
    archdep_thread_init();

    mainlock_init();

    main_loop_forever();

    /*
     * main_loop_forever() does not return, so we call archdep_thread_shutdown()
     * in the mainlock system which manages a direct pthread based thread exit.
     */

    return NULL;
}

#endif /* #ifdef USE_VICE_THREAD */

/*
 * ffconf.h — FatFS R0.12c configuration for STM32F405 flight controller.
 *
 * Version token: _FFCONF must equal 68300 to match ff.h's _FATFS check.
 * (R0.12c uses underscore-prefix macros throughout.)
 *
 * LFN mode 1 (static BSS buffer): enabled, no heap/RTOS needed.
 * Requires option/unicode.c to be compiled — handled by add_usb_middleware.py.
 *
 * _FS_REENTRANT is disabled: no RTOS on this firmware.
 * To enable, provide ff_req_grant()/ff_rel_grant() CMSIS-OS wrappers.
 */

#define _FFCONF     68300       /* MUST match _FATFS in ff.h — do not change */

/*---------------------------------------------------------------------------/
/ Function Configurations
/---------------------------------------------------------------------------*/

#define _FS_READONLY    0
#define _FS_MINIMIZE    0
#define _USE_STRFUNC    0
#define _USE_FIND       0
#define _USE_MKFS       1
#define _USE_FASTSEEK   1
#define _USE_EXPAND     0
#define _USE_CHMOD      0
#define _USE_LABEL      0
#define _USE_FORWARD    0

/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/---------------------------------------------------------------------------*/

#define _CODE_PAGE      437         /* US ASCII — smallest SBCS table       */

#define _USE_LFN        1           /* 1 = LFN with static BSS buffer       */
#define _MAX_LFN        255
#define _LFN_UNICODE    0           /* 0 = ANSI/OEM strings on the API      */
#define _STRF_ENCODE    3
#define _FS_RPATH       0

/* LFN mode 1 uses a static BSS buffer — not thread-safe but fine for
   bare-metal single-threaded firmware.  unicode.c in option/ is required
   and is added by scripts/add_usb_middleware.py. */
#if _USE_LFN == 3
#include <stdlib.h>
#define ff_malloc   malloc
#define ff_free     free
#endif

/*---------------------------------------------------------------------------/
/ Drive/Volume Configurations
/---------------------------------------------------------------------------*/

#define _VOLUMES            1
#define _STR_VOLUME_ID      0
#define _VOLUME_STRS        "SD"
#define _MULTI_PARTITION    0
#define _MIN_SS             512
#define _MAX_SS             512
#define _USE_TRIM           0
#define _FS_NOFSINFO        0

/*---------------------------------------------------------------------------/
/ System Configurations
/---------------------------------------------------------------------------*/

#define _FS_TINY        0
#define _FS_EXFAT       0

#define _FS_NORTC       1           /* No RTC — use fixed timestamp          */
#define _NORTC_MON      1
#define _NORTC_MDAY     1
#define _NORTC_YEAR     2025

#define _FS_LOCK        2           /* Up to 2 simultaneously open objects   */

#define _FS_REENTRANT   0           /* No RTOS — single-threaded bare-metal  */
#define _USE_MUTEX      0

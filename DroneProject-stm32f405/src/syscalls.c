/**
 * @file    syscalls.c
 * @brief   Newlib syscall stubs — redirect printf() to USB CDC
 *
 * When printf() is called, newlib calls _write(). We forward that to
 * CDC_Transmit_FS() so everything appears on the host's serial monitor
 * without any UART hardware.
 *
 * Safety:
 *  - Checks USBD_STATE_CONFIGURED before transmitting (prevents hang
 *    if USB cable is disconnected).
 *  - Returns len even on BUSY — newlib must not think bytes were lost.
 */

#include "stm32f4xx_hal.h"
#include "usb/usb_device.h"
#include "usb/usbd_cdc_if.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* ── Minimal stubs required by newlib ───────────────────────────── */

int _close(int file)   { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _isatty(int file)  { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }

/* ── printf → USB CDC ────────────────────────────────────────────── */

int _write(int file, char *ptr, int len)
{
    (void)file;

    /* Only transmit when host has configured the device */
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
        return len;   /* silently discard — not connected */
    }

    /* Best-effort: if USB is busy with previous transfer, drop this one.
     * Use a small retry loop (max 5 ms) to handle temporary busy states. */
    uint32_t retries = 5U;
    while (retries--) {
        if (CDC_Transmit_FS((uint8_t*)ptr, (uint16_t)len) == USBD_OK) {
            break;
        }
        HAL_Delay(1U);
    }

    return len;
}

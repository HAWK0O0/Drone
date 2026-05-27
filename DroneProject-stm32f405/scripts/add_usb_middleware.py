"""
add_usb_middleware.py — PlatformIO pre-build extra script

Injects two ST middleware stacks from framework-stm32cubef4:
  1. STM32 USB Device Library (Core + CDC class)
  2. FatFS (Third_Party/FatFs) — only ff.c, custom disk I/O in src/drivers/

Why this is needed:
  PlatformIO's build_src_filter only covers files inside the project.
  ST middleware lives inside the framework package directory, so we must
  locate and inject sources + include paths at build-time.
"""

Import("env")  # noqa: F821 — PlatformIO SCons namespace
import os

fw_dir = env.PioPlatform().get_package_dir("framework-stm32cubef4")
if not fw_dir:
    print("[MW] WARNING: framework-stm32cubef4 not found — skipping USB + FatFS")
    Return()  # noqa: F821

# ── 1. STM32 USB Device Library ─────────────────────────────────────────────
mw_dir   = os.path.join(fw_dir, "Middlewares", "ST", "STM32_USB_Device_Library")
core_src = os.path.join(mw_dir, "Core", "Src")
cdc_src  = os.path.join(mw_dir, "Class", "CDC", "Src")
core_inc = os.path.join(mw_dir, "Core", "Inc")
cdc_inc  = os.path.join(mw_dir, "Class", "CDC", "Inc")

usb_ok = all(os.path.isdir(d) for d in [core_src, cdc_src, core_inc, cdc_inc])
if not usb_ok:
    print(f"[USB] WARNING: middleware not found under {mw_dir} — skipping USB")
else:
    env.Append(CPPPATH=[core_inc, cdc_inc])
    # Exclude *_template.c — conflicts with our custom src/usb/ implementations
    env.BuildSources(
        os.path.join("$BUILD_DIR", "usb_mw_core"),
        core_src,
        src_filter="+<*> -<*_template.c>"
    )
    env.BuildSources(
        os.path.join("$BUILD_DIR", "usb_mw_cdc"),
        cdc_src,
        src_filter="+<*> -<*_template.c>"
    )
    print(f"[USB] Middleware injected from: {mw_dir}")

# ── 2. FatFS ─────────────────────────────────────────────────────────────────
fatfs_src = os.path.join(fw_dir, "Middlewares", "Third_Party", "FatFs", "src")
if not os.path.isdir(fatfs_src):
    print(f"[FatFS] WARNING: Not found at {fatfs_src} — SD blackbox disabled")
else:
    # Add FatFS header path (ff.h, diskio.h).
    # Our include/ dir is listed first via -Iinclude in build_flags, so our
    # ffconf.h takes precedence over any template inside the framework package.
    env.Append(CPPPATH=[fatfs_src])
    # Build only ff.c + option/unicode.c (required for _USE_LFN != 0).
    # Skip diskio.c (template stub — we provide src/drivers/sd_diskio.c),
    # ffconf_template.h is a header and not compiled, but the option/ codepage
    # tables (cc*.c, ccsbcs.c) are #include'd directly by unicode.c — no need
    # to add them separately.
    env.BuildSources(
        os.path.join("$BUILD_DIR", "fatfs_mw"),
        fatfs_src,
        src_filter="+<ff.c> +<option/unicode.c> -<diskio.c> -<*_template*> -<ff_gen_drv.c>"
    )
    print(f"[FatFS] Middleware injected from: {fatfs_src}")

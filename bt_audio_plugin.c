#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include <orbis/Pad.h>
#include "plugin_common.h"

#define LOG_PATH GOLDHEN_PATH "/plugins/bt_audio_debug.log"
#define RX20_MAC "80:D2:72:5A:C2:4D"

/*
 * This build contains the controller, notification, logging and pass-through
 * parts of BlueHEN. Bluetooth inquiry/A2DP is deliberately not faked: the
 * workspace has no PS4-compatible BTstack or SBC implementation.
 */
attr_public const char *g_pluginName = "bluehen";
attr_public const char *g_pluginDesc = "Bluetooth audio controller (BT backend pending)";
attr_public const char *g_pluginAuth = "BlueHEN";
attr_public u32 g_pluginVersion = 0x00000100;

static volatile int g_enabled;
static volatile int g_scanning;
static uint32_t g_previous_buttons;

HOOK_INIT(scePadRead);

static void log_debug_ms(const char *level, const char *fmt, ...)
{
    struct timeval tv;
    struct tm *local_time;
    char line[1024];
    char timestamp[64];
    va_list args;
    int fd;
    int length;

    if (gettimeofday(&tv, NULL) != 0) {
        return;
    }

    local_time = localtime(&tv.tv_sec);
    if (local_time == NULL ||
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time) == 0) {
        return;
    }
    snprintf(timestamp + strlen(timestamp), sizeof(timestamp) - strlen(timestamp),
             ".%03ld", (long)(tv.tv_usec / 1000));
    length = snprintf(line, sizeof(line), "[%s] [%s] ", timestamp, level);
    if (length < 0 || (size_t)length >= sizeof(line)) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(line + length, sizeof(line) - (size_t)length, fmt, args);
    va_end(args);

    fd = open(LOG_PATH, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
        return;
    }

    write(fd, line, strlen(line));
    write(fd, "\n", 1);
    fsync(fd);
    close(fd);
}

static void notify(const char *message)
{
    NotifyStatic(TEX_ICON_SYSTEM, message);
    log_debug_ms("NOTIFY", "%s", message);
}

static uint32_t pressed(uint32_t buttons)
{
    return buttons & ~g_previous_buttons;
}

static void handle_shortcuts(uint32_t buttons)
{
    uint32_t newly_pressed = pressed(buttons);
    const uint32_t sticks = ORBIS_PAD_BUTTON_L3 | ORBIS_PAD_BUTTON_R3;

    if ((buttons & sticks) != sticks) {
        return;
    }

    if (newly_pressed & ORBIS_PAD_BUTTON_TRIANGLE) {
        g_enabled = !g_enabled;
        if (g_enabled) {
            log_debug_ms("STATE", "Master switch enabled");
            notify("[BlueHEN] Enabled");
        } else {
            log_debug_ms("STATE", "Master switch disabled; audio pass-through active");
            notify("[BlueHEN] Disabled; audio pass-through active");
        }
    }

    if (!g_enabled) {
        return;
    }

    if (newly_pressed & ORBIS_PAD_BUTTON_SQUARE) {
        g_scanning = 1;
        log_debug_ms("BT_GAP", "Inquiry requested for 8 seconds");
        notify("[BlueHEN] Inquiry requested, but BT backend is not installed");
    }

    if (newly_pressed & ORBIS_PAD_BUTTON_CROSS) {
        if (g_scanning) {
            g_scanning = 0;
            log_debug_ms("CONNECT", "Connect requested after inquiry; no BT backend available");
            notify("[BlueHEN] No BT backend; cannot connect");
        } else {
            log_debug_ms("CONNECT", "Connect requested for RX20 %s; no BT backend available", RX20_MAC);
            notify("[BlueHEN] BT backend unavailable; RX20 not connected");
        }
    }
}

int32_t scePadRead_hook(int32_t handle, OrbisPadData *data, int32_t count)
{
    int32_t result = HOOK_CONTINUE(scePadRead,
                                   int32_t (*)(int32_t, OrbisPadData *, int32_t),
                                   handle, data, count);

    if (result >= 0 && data != NULL) {
        handle_shortcuts(data->buttons);
        g_previous_buttons = data->buttons;
    }

    return result;
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    g_enabled = 0;
    g_scanning = 0;
    g_previous_buttons = 0;
    boot_ver();
    log_debug_ms("INIT", "BlueHEN loaded; disabled by default");
    log_debug_ms("INIT", "Bluetooth backend unavailable; audio remains untouched");
    HOOK32(scePadRead);
    notify("[BlueHEN] Loaded; L3+R3+TRIANGLE enables");
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    UNHOOK(scePadRead);
    log_debug_ms("STOP", "BlueHEN unloaded");
    return 0;
}

s32 attr_module_hidden module_start(s64 argc, const void *args)
{
    (void)argc;
    (void)args;
    return 0;
}

s32 attr_module_hidden module_stop(s64 argc, const void *args)
{
    (void)argc;
    (void)args;
    return 0;
}

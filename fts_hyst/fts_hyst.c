/*
 * Copyright (C) 2026 The 2by2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * fts_hyst - fix touch chattering of the FocalTech FT3683G touch IC.
 *
 * The IC firmware uses a single threshold (reg 0x80, in units of 4 diff
 * counts) for both the touch-down and the touch-up decision.  Weak contacts
 * (a fingernail, or a finger lifting off at the end of a swipe) produce a
 * signal hovering right at that threshold, so the IC flips between DOWN and
 * UP every frame and the screen gets tapped repeatedly.
 *
 * 1. Hysteresis: reg 0x80 is lowered by DELTA while any finger is down and
 *    restored once all fingers are up, so the press threshold is unchanged
 *    and the release threshold becomes (thr - DELTA) * 4.
 * 2. Debounce: the firmware latches the threshold when a scan starts, so the
 *    lowered value only applies one frame after the first DOWN.  A weak
 *    contact can therefore still be released on its very first frame and
 *    pressed again right after.  The touchscreen is grabbed and re-emitted
 *    through uinput: the release of a contact shorter than SHORT_TOUCH_MS is
 *    held back for up to HOLD_MS, and if the finger comes back in the same
 *    slot meanwhile both contacts are merged into one.  DOWN is never
 *    delayed and nothing is dropped, so the sensitivity is unchanged.
 *
 * The IC reloads its default threshold on every reset and the grab is
 * released when this daemon exits, so without it the touch behaves exactly
 * like stock.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define PROC_PATH           "/proc/ftxxxx-debug"
#define PROC_READ_REGISTER  1
#define PROC_WRITE_REGISTER 2
#define REG_THGROUP         0x80
#define THR_MIN             0x10
#define THR_MAX             0x7F
#define DEFAULT_DELTA       16
#define RT_PRIORITY         50
#define SHORT_TOUCH_MS      15
#define HOLD_MS             40

#define MAX_SLOTS           16
#define BITS_LONG           (sizeof(long) * 8)
#define NLONGS(x)           (((x) + BITS_LONG - 1) / BITS_LONG)
#define TEST_BIT(b, a)      (((a)[(b) / BITS_LONG] >> ((b) % BITS_LONG)) & 1)

/* MT axes mirrored per slot; tracking id first so a new contact starts it */
static const int mt_codes[] = {
    ABS_MT_TRACKING_ID, ABS_MT_TOOL_TYPE,   ABS_MT_POSITION_X,
    ABS_MT_POSITION_Y,  ABS_MT_TOUCH_MAJOR, ABS_MT_PRESSURE,
};
#define NUM_MT (sizeof(mt_codes) / sizeof(mt_codes[0]))

struct touch_state {
    int mt[MAX_SLOTS][NUM_MT];
    int touch;
};

static int proc_fd = -1, ui_fd = -1;
static int num_slots = 10;
static int has_code[NUM_MT];
static volatile sig_atomic_t stop;

static int reg_read(unsigned char reg, unsigned char *val) {
    unsigned char cmd[2] = {PROC_READ_REGISTER, reg};

    if (write(proc_fd, cmd, sizeof(cmd)) != sizeof(cmd)) return -1;
    if (read(proc_fd, val, 1) != 1) return -1;
    return 0;
}

static int reg_write(unsigned char reg, unsigned char val) {
    unsigned char cmd[3] = {PROC_WRITE_REGISTER, reg, val};

    return write(proc_fd, cmd, sizeof(cmd)) == sizeof(cmd) ? 0 : -1;
}

static long long now_ms(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

/* Find the real touchscreen, not the uinput clone of it (BUS_VIRTUAL). */
static int open_touch_input(const char *want) {
    char path[64], name[64];
    struct input_id id;
    struct dirent *e;
    DIR *d = opendir("/dev/input");
    int found = -1;

    if (!d) return -1;
    while (found < 0 && (e = readdir(d))) {
        if (strncmp(e->d_name, "event", 5)) continue;
        snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
        int fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0) continue;
        memset(name, 0, sizeof(name));
        if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0 && !strcmp(name, want) &&
            ioctl(fd, EVIOCGID, &id) >= 0 && id.bustype != BUS_VIRTUAL)
            found = fd;
        else
            close(fd);
    }
    closedir(d);
    return found;
}

static int create_clone(int evfd) {
    unsigned long evbits[NLONGS(EV_CNT)] = {0}, keybits[NLONGS(KEY_CNT)] = {0};
    unsigned long absbits[NLONGS(ABS_CNT)] = {0}, propbits[NLONGS(INPUT_PROP_CNT)] = {0};
    struct uinput_setup us;
    int fd;

    fd = open("/dev/uinput", O_WRONLY | O_CLOEXEC);
    if (fd < 0) return -1;

    ioctl(evfd, EVIOCGBIT(0, sizeof(evbits)), evbits);
    ioctl(evfd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
    ioctl(evfd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits);
    ioctl(evfd, EVIOCGPROP(sizeof(propbits)), propbits);

    for (int i = 0; i < EV_CNT; i++)
        if (TEST_BIT(i, evbits)) ioctl(fd, UI_SET_EVBIT, i);
    for (int i = 0; i < KEY_CNT; i++)
        if (TEST_BIT(i, keybits)) ioctl(fd, UI_SET_KEYBIT, i);
    for (int i = 0; i < INPUT_PROP_CNT; i++)
        if (TEST_BIT(i, propbits)) ioctl(fd, UI_SET_PROPBIT, i);
    for (int i = 0; i < ABS_CNT; i++) {
        struct uinput_abs_setup abs = {.code = i};
        if (!TEST_BIT(i, absbits) || ioctl(evfd, EVIOCGABS(i), &abs.absinfo) < 0) continue;
        ioctl(fd, UI_SET_ABSBIT, i);
        ioctl(fd, UI_ABS_SETUP, &abs);
        if (i == ABS_MT_SLOT) num_slots = abs.absinfo.maximum + 1;
        for (size_t c = 0; c < NUM_MT; c++)
            if (mt_codes[c] == i) has_code[c] = 1;
    }
    if (num_slots > MAX_SLOTS) num_slots = MAX_SLOTS;

    memset(&us, 0, sizeof(us));
    ioctl(evfd, EVIOCGID, &us.id);
    us.id.bustype = BUS_VIRTUAL;
    ioctl(evfd, EVIOCGNAME(sizeof(us.name) - 1), us.name);
    if (ioctl(fd, UI_DEV_SETUP, &us) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void emit(int type, int code, int value) {
    struct input_event ev = {.type = type, .code = code, .value = value};

    write(ui_fd, &ev, sizeof(ev));
}

/* Bring the clone up to date with the real device and send a frame. */
/* A raw contact that continues a held contact of the clone. */
static struct {
    int slot, raw_id, clone_id;
} alias = {-1, -1, -1};

static void sync_clone(struct touch_state *clone, const struct touch_state *raw,
                       int *clone_slot) {
    for (int s = 0; s < num_slots; s++) {
        for (size_t c = 0; c < NUM_MT; c++) {
            int want = raw->mt[s][c];

            if (c == 0 && s == alias.slot) {
                if (want == alias.raw_id)
                    want = alias.clone_id;
                else
                    alias.slot = -1; /* merged contact ended */
            }
            if (!has_code[c] || clone->mt[s][c] == want) continue;
            if (*clone_slot != s) {
                emit(EV_ABS, ABS_MT_SLOT, s);
                *clone_slot = s;
            }
            emit(EV_ABS, mt_codes[c], want);
            clone->mt[s][c] = want;
        }
    }
    if (clone->touch != raw->touch) {
        emit(EV_KEY, BTN_TOUCH, raw->touch);
        clone->touch = raw->touch;
    }
    emit(EV_SYN, SYN_REPORT, 0);
}

/* Return the only active slot, -1 if none or more than one. */
static int single_slot(const struct touch_state *st) {
    int found = -1;

    for (int s = 0; s < num_slots; s++) {
        if (st->mt[s][0] < 0) continue;
        if (found >= 0) return -1;
        found = s;
    }
    return found;
}

static void resync_raw(int evfd, struct touch_state *raw) {
    struct {
        __u32 code;
        __s32 values[MAX_SLOTS];
    } req;
    unsigned long keys[NLONGS(KEY_CNT)] = {0};

    for (size_t c = 0; c < NUM_MT; c++) {
        if (!has_code[c]) continue;
        req.code = mt_codes[c];
        if (ioctl(evfd, EVIOCGMTSLOTS(sizeof(req)), &req) < 0) continue;
        for (int s = 0; s < num_slots; s++) raw->mt[s][c] = req.values[s];
    }
    if (ioctl(evfd, EVIOCGKEY(sizeof(keys)), keys) >= 0) raw->touch = TEST_BIT(BTN_TOUCH, keys);
}

static void on_signal(int sig) {
    (void)sig;
    stop = 1;
}

int main(int argc, char **argv) {
    enum { IDLE, ACTIVE, HOLD } state = IDLE;
    int delta = argc > 1 ? atoi(argv[1]) : DEFAULT_DELTA;
    unsigned char hi = 0, lo = 0, cur = 0;
    struct touch_state raw, clone;
    struct input_event ev[64];
    struct sigaction sa;
    long long down_at = 0, hold_until = 0;
    int raw_slot = 0, clone_slot = -1, raw_down = 0, dropped = 0;

    if (delta <= 0 || delta > THR_MAX) delta = DEFAULT_DELTA;

    int evfd = open_touch_input("fts_ts");
    proc_fd = open(PROC_PATH, O_RDWR | O_CLOEXEC);
    if (evfd < 0 || proc_fd < 0) {
        fprintf(stderr, "fts_hyst: touch device not available\n");
        return 1;
    }
    if (reg_read(REG_THGROUP, &hi) || hi < THR_MIN || hi > THR_MAX) {
        fprintf(stderr, "fts_hyst: unexpected threshold 0x%02x\n", hi);
        return 1;
    }

    /* Without uinput, fall back to hysteresis only on the shared device. */
    ui_fd = create_clone(evfd);
    if (ui_fd < 0 || ioctl(evfd, EVIOCGRAB, 1) < 0) {
        fprintf(stderr, "fts_hyst: debounce unavailable: %s\n", strerror(errno));
        if (ui_fd >= 0) close(ui_fd);
        ui_fd = -1;
    }

    /* A freshly created uinput device starts with no contacts and zeroed axes. */
    memset(&clone, 0, sizeof(clone));
    for (int s = 0; s < MAX_SLOTS; s++) clone.mt[s][0] = -1;
    /*
     * The kernel only reports changed values, e.g. the constant pressure is
     * sent once after boot, so start from the device's current state and let
     * the first sync push everything to the clone.
     */
    raw = clone;
    resync_raw(evfd, &raw);

    struct sched_param sp = {.sched_priority = RT_PRIORITY};
    if (sched_setscheduler(0, SCHED_FIFO, &sp))
        fprintf(stderr, "fts_hyst: failed to set SCHED_FIFO: %s\n", strerror(errno));

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    while (!stop) {
        struct pollfd pfd = {.fd = evfd, .events = POLLIN};
        int timeout = -1;

        if (state == HOLD) {
            long long left = hold_until - now_ms();
            timeout = left > 0 ? (int)left : 0;
        }
        int pr = poll(&pfd, 1, timeout);
        if (pr < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (pr == 0) {
            /* The finger did not come back: deliver the held release. */
            if (state == HOLD) {
                sync_clone(&clone, &raw, &clone_slot);
                if (!raw_down) reg_write(REG_THGROUP, hi);
                state = IDLE;
            }
            continue;
        }

        ssize_t n = read(evfd, ev, sizeof(ev));
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            break;
        }
        for (size_t i = 0; i < (size_t)n / sizeof(ev[0]); i++) {
            const struct input_event *e = &ev[i];

            if (dropped) {
                if (e->type == EV_SYN && e->code == SYN_REPORT) {
                    resync_raw(evfd, &raw);
                    dropped = 0;
                } else {
                    continue;
                }
            } else if (e->type == EV_ABS) {
                if (e->code == ABS_MT_SLOT) {
                    raw_slot = e->value;
                } else if (raw_slot >= 0 && raw_slot < num_slots) {
                    for (size_t c = 0; c < NUM_MT; c++)
                        if (mt_codes[c] == e->code) raw.mt[raw_slot][c] = e->value;
                }
                continue;
            } else if (e->type == EV_KEY) {
                if (e->code == BTN_TOUCH)
                    raw.touch = e->value;
                else if (ui_fd >= 0)
                    emit(EV_KEY, e->code, e->value); /* gesture keys etc. */
                continue;
            } else if (e->type == EV_SYN && e->code == SYN_DROPPED) {
                dropped = 1;
                continue;
            } else if (!(e->type == EV_SYN && e->code == SYN_REPORT)) {
                continue;
            }

            /* End of a frame from the IC. */
            if (raw.touch && !raw_down) {
                raw_down = 1;
                /* Re-read the default, the IC restores it on every reset. */
                if (!reg_read(REG_THGROUP, &cur) && cur != lo && cur >= THR_MIN && cur <= THR_MAX)
                    hi = cur;
                lo = hi > THR_MIN + delta ? hi - delta : THR_MIN;
                reg_write(REG_THGROUP, lo);
            } else if (!raw.touch && raw_down) {
                raw_down = 0;
                /* While a short contact is held, keep it easy to come back. */
                if (ui_fd < 0 || state != ACTIVE || now_ms() - down_at >= SHORT_TOUCH_MS ||
                    single_slot(&clone) < 0)
                    reg_write(REG_THGROUP, hi);
            }

            if (ui_fd < 0) continue;
            switch (state) {
                case IDLE:
                    sync_clone(&clone, &raw, &clone_slot);
                    if (raw.touch) {
                        state = ACTIVE;
                        down_at = now_ms();
                    }
                    break;
                case ACTIVE:
                    if (!raw.touch && now_ms() - down_at < SHORT_TOUCH_MS &&
                        single_slot(&clone) >= 0) {
                        /* Possible first-frame dropout: hold the release back. */
                        state = HOLD;
                        hold_until = now_ms() + HOLD_MS;
                        break;
                    }
                    sync_clone(&clone, &raw, &clone_slot);
                    if (!raw.touch) state = IDLE;
                    break;
                case HOLD: {
                    if (!raw.touch) break;
                    int cs = single_slot(&clone), rs = single_slot(&raw);
                    if (cs >= 0 && cs == rs) {
                        /* Same finger came back: continue the held contact. */
                        alias.slot = rs;
                        alias.raw_id = raw.mt[rs][0];
                        alias.clone_id = clone.mt[cs][0];
                    } else {
                        /* Something else: release the held contact first. */
                        struct touch_state up = raw;
                        for (int sl = 0; sl < num_slots; sl++) up.mt[sl][0] = -1;
                        up.touch = 0;
                        sync_clone(&clone, &up, &clone_slot);
                    }
                    sync_clone(&clone, &raw, &clone_slot);
                    state = ACTIVE;
                    break;
                }
            }
        }
    }

    if (raw_down) reg_write(REG_THGROUP, hi);
    if (ui_fd >= 0) {
        ioctl(evfd, EVIOCGRAB, 0);
        ioctl(ui_fd, UI_DEV_DESTROY);
    }
    return 0;
}

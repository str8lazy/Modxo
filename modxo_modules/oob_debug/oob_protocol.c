/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Line-oriented control protocol. This file is written to also build on a host
(with OOB_HOST_TEST) so parsing / confirm gating can be unit-tested without
hardware.
*/
#include <oob_debug.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OOB_HOST_TEST
#include <pico/stdlib.h>
#include <modxo_pinout.h>
#ifndef XBOX_SMBUS_SDA
#define XBOX_SMBUS_SDA EXPANSION_PORT_I2C_SDA
#endif
#ifndef XBOX_SMBUS_SCL
#define XBOX_SMBUS_SCL EXPANSION_PORT_I2C_SCL
#endif
#endif

static const char *smbus_err_str(smbus_result_t r)
{
    switch (r) {
    case SMBUS_OK: return "ok";
    case SMBUS_ERR_TIMEOUT: return "timeout";
    case SMBUS_ERR_NACK: return "nack";
    case SMBUS_ERR_ARB_LOST: return "arb_lost";
    case SMBUS_ERR_BUS_BUSY: return "bus_busy";
    case SMBUS_ERR_INVALID: return "invalid";
    default: return "error";
    }
}

static void skip_ws(const char **p)
{
    while (**p && isspace((unsigned char)**p)) {
        (*p)++;
    }
}

static int match_cmd(const char **p, const char *cmd)
{
    size_t n = strlen(cmd);
    if (strncasecmp(*p, cmd, n) != 0) {
        return 0;
    }
    char next = (*p)[n];
    if (next != '\0' && !isspace((unsigned char)next)) {
        return 0;
    }
    *p += n;
    skip_ws(p);
    return 1;
}

static int parse_u8(const char **p, uint8_t *out)
{
    char *end = NULL;
    skip_ws(p);
    unsigned long v = strtoul(*p, &end, 0);
    if (end == *p || v > 0xFFul) {
        return 0;
    }
    *out = (uint8_t)v;
    *p = end;
    skip_ws(p);
    return 1;
}

static int parse_u32(const char **p, uint32_t *out)
{
    char *end = NULL;
    skip_ws(p);
    unsigned long v = strtoul(*p, &end, 0);
    if (end == *p) {
        return 0;
    }
    *out = (uint32_t)v;
    *p = end;
    skip_ws(p);
    return 1;
}

static bool has_confirm(const char *p)
{
    skip_ws(&p);
    if (*p == '\0') {
        return false;
    }
    if (strcasecmp(p, OOB_CONFIRM_TOKEN) == 0) {
        return true;
    }
    /* Also accept --confirm as a flag-style token */
    if (strcasecmp(p, "--confirm") == 0) {
        return true;
    }
    return false;
}

static void fill_help(oob_cmd_result_t *out)
{
    snprintf(out->reply, sizeof(out->reply),
             "OK commands:\r\n"
             "  help\r\n"
             "  status\r\n"
             "  heartbeat\r\n"
             "  hb-stale <ms>     (0 disables unsolicited HB-STALE)\r\n"
             "  smbus scan\r\n"
             "  smbus read <addr7> <reg>\r\n"
             "  smbus write <addr7> <reg> <val>\r\n"
             "  reset CONFIRM    (PIC 0x10 reg 0x02 = 0x01)\r\n"
             "  cycle CONFIRM    (PIC 0x10 reg 0x02 = 0x40)\r\n"
             "  off CONFIRM      (PIC 0x10 reg 0x02 = 0x80)\r\n"
             "Notes: addr is 7-bit hex/dec; destructive cmds need CONFIRM.\r\n"
             "Heartbeat LPC port: 0x%04X\r\n",
             LPC_HEARTBEAT_PORT);
    out->ok = true;
}

#ifndef OOB_HOST_TEST
static void fill_status(oob_cmd_result_t *out)
{
    const heartbeat_state_t *hb = heartbeat_get_state();
    uint32_t now = time_us_32();
    uint32_t hb_age = hb->have_write ? (now - hb->last_write_us) / 1000u : 0xFFFFFFFFu;
    uint32_t lpc_age = (now - hb->last_lpc_activity_us) / 1000u;

    uint8_t pic_ver = 0;
    uint8_t tcpu = 0;
    uint8_t tamb = 0;
    smbus_result_t rv = smbus_read_byte(XBOX_PIC_ADDR, PIC_REG_VERSION, &pic_ver);
    smbus_result_t rc = smbus_read_byte(XBOX_PIC_ADDR, PIC_REG_TEMP_CPU, &tcpu);
    smbus_result_t ra = smbus_read_byte(XBOX_PIC_ADDR, PIC_REG_TEMP_AMB, &tamb);

    snprintf(out->reply, sizeof(out->reply),
             "OK status\r\n"
             "  pic_version=0x%02X (%s)\r\n"
             "  temp_cpu=%u (%s) temp_amb=%u (%s)\r\n"
             "  heartbeat_port=0x%04X last=0x%02X age_ms=%u have=%u\r\n"
             "  lpc_age_ms=%u hb_stale_ms=%u\r\n"
             "  smbus_pins=GP%u/GP%u (open-drain, no pico pull-ups)\r\n",
             pic_ver, smbus_err_str(rv),
             tcpu, smbus_err_str(rc), tamb, smbus_err_str(ra),
             LPC_HEARTBEAT_PORT, hb->last_value, hb_age, (unsigned)hb->have_write,
             lpc_age, hb->stale_timeout_ms,
             (unsigned)XBOX_SMBUS_SDA, (unsigned)XBOX_SMBUS_SCL);
    out->ok = true;
    out->needs_smbus = true;
}

static void fill_heartbeat(oob_cmd_result_t *out)
{
    const heartbeat_state_t *hb = heartbeat_get_state();
    uint32_t now = time_us_32();
    uint32_t hb_age = hb->have_write ? (now - hb->last_write_us) / 1000u : 0xFFFFFFFFu;
    uint32_t lpc_age = (now - hb->last_lpc_activity_us) / 1000u;
    snprintf(out->reply, sizeof(out->reply),
             "OK heartbeat last=0x%02X age_ms=%u have=%u lpc_age_ms=%u stale_ms=%u\r\n",
             hb->last_value, hb_age, (unsigned)hb->have_write, lpc_age, hb->stale_timeout_ms);
    out->ok = true;
}

static void do_scan(oob_cmd_result_t *out)
{
    char *p = out->reply;
    size_t left = sizeof(out->reply);
    int n = snprintf(p, left, "OK smbus scan:");
    if (n < 0 || (size_t)n >= left) {
        out->ok = false;
        return;
    }
    p += n;
    left -= (size_t)n;

    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (smbus_probe(addr) == SMBUS_OK) {
            n = snprintf(p, left, " 0x%02X", addr);
            if (n < 0 || (size_t)n >= left) {
                break;
            }
            p += n;
            left -= (size_t)n;
        }
    }
    snprintf(p, left, "\r\n");
    out->ok = true;
    out->needs_smbus = true;
}

static void do_power(oob_cmd_result_t *out, uint8_t cmd, const char *name, const char *rest)
{
    if (!has_confirm(rest)) {
        snprintf(out->reply, sizeof(out->reply),
                 "ERR %s requires CONFIRM (e.g. '%s CONFIRM')\r\n", name, name);
        out->ok = false;
        return;
    }
    smbus_result_t r = smbus_write_byte(XBOX_PIC_ADDR, PIC_REG_POWER_CTRL, cmd);
    if (r == SMBUS_OK) {
        snprintf(out->reply, sizeof(out->reply),
                 "OK %s wrote PIC 0x%02X reg 0x%02X val 0x%02X\r\n",
                 name, XBOX_PIC_ADDR, PIC_REG_POWER_CTRL, cmd);
        out->ok = true;
    } else {
        snprintf(out->reply, sizeof(out->reply), "ERR %s failed: %s\r\n", name, smbus_err_str(r));
        out->ok = false;
    }
    out->needs_smbus = true;
}
#else
/* Host-test stubs: no SMBus, still exercise confirm gating and parsing. */
static void fill_status(oob_cmd_result_t *out)
{
    snprintf(out->reply, sizeof(out->reply), "OK status (host-test stub)\r\n");
    out->ok = true;
}
static void fill_heartbeat(oob_cmd_result_t *out)
{
    snprintf(out->reply, sizeof(out->reply), "OK heartbeat (host-test stub)\r\n");
    out->ok = true;
}
static void do_scan(oob_cmd_result_t *out)
{
    snprintf(out->reply, sizeof(out->reply), "OK smbus scan: (host-test stub)\r\n");
    out->ok = true;
}
static void do_power(oob_cmd_result_t *out, uint8_t cmd, const char *name, const char *rest)
{
    (void)cmd;
    if (!has_confirm(rest)) {
        snprintf(out->reply, sizeof(out->reply),
                 "ERR %s requires CONFIRM (e.g. '%s CONFIRM')\r\n", name, name);
        out->ok = false;
        return;
    }
    snprintf(out->reply, sizeof(out->reply), "OK %s (host-test stub)\r\n", name);
    out->ok = true;
}
#endif

bool oob_parse_and_dispatch(const char *line, oob_cmd_result_t *out, bool execute)
{
    if (!line || !out) {
        return false;
    }
    memset(out, 0, sizeof(*out));

    const char *p = line;
    skip_ws(&p);
    if (*p == '\0') {
        return false;
    }

    if (match_cmd(&p, "help") || match_cmd(&p, "?")) {
        if (execute) {
            fill_help(out);
        } else {
            snprintf(out->reply, sizeof(out->reply), "OK help\r\n");
            out->ok = true;
        }
        return true;
    }

    if (match_cmd(&p, "status")) {
        if (execute) {
            fill_status(out);
        } else {
            snprintf(out->reply, sizeof(out->reply), "OK status\r\n");
            out->ok = true;
        }
        return true;
    }

    if (match_cmd(&p, "heartbeat")) {
        if (execute) {
            fill_heartbeat(out);
        } else {
            snprintf(out->reply, sizeof(out->reply), "OK heartbeat\r\n");
            out->ok = true;
        }
        return true;
    }

    if (match_cmd(&p, "hb-stale")) {
        uint32_t ms = 0;
        if (!parse_u32(&p, &ms)) {
            snprintf(out->reply, sizeof(out->reply), "ERR usage: hb-stale <ms>\r\n");
            out->ok = false;
            return true;
        }
        if (execute) {
#ifndef OOB_HOST_TEST
            heartbeat_set_stale_timeout_ms(ms);
#endif
        }
        snprintf(out->reply, sizeof(out->reply), "OK hb-stale %u\r\n", ms);
        out->ok = true;
        return true;
    }

    if (match_cmd(&p, "smbus")) {
        if (match_cmd(&p, "scan")) {
            if (execute) {
                do_scan(out);
            } else {
                snprintf(out->reply, sizeof(out->reply), "OK smbus scan\r\n");
                out->ok = true;
            }
            return true;
        }
        if (match_cmd(&p, "read")) {
            uint8_t addr = 0, reg = 0;
            if (!parse_u8(&p, &addr) || !parse_u8(&p, &reg)) {
                snprintf(out->reply, sizeof(out->reply), "ERR usage: smbus read <addr> <reg>\r\n");
                out->ok = false;
                return true;
            }
            if (!execute) {
                snprintf(out->reply, sizeof(out->reply), "OK smbus read 0x%02X 0x%02X\r\n", addr, reg);
                out->ok = true;
                return true;
            }
#ifndef OOB_HOST_TEST
            uint8_t val = 0;
            smbus_result_t r = smbus_read_byte(addr, reg, &val);
            if (r == SMBUS_OK) {
                snprintf(out->reply, sizeof(out->reply), "OK read 0x%02X 0x%02X = 0x%02X\r\n", addr, reg, val);
                out->ok = true;
            } else {
                snprintf(out->reply, sizeof(out->reply), "ERR read: %s\r\n", smbus_err_str(r));
                out->ok = false;
            }
            out->needs_smbus = true;
#else
            snprintf(out->reply, sizeof(out->reply), "OK read 0x%02X 0x%02X = 0x00\r\n", addr, reg);
            out->ok = true;
#endif
            return true;
        }
        if (match_cmd(&p, "write")) {
            uint8_t addr = 0, reg = 0, val = 0;
            if (!parse_u8(&p, &addr) || !parse_u8(&p, &reg) || !parse_u8(&p, &val)) {
                snprintf(out->reply, sizeof(out->reply),
                         "ERR usage: smbus write <addr> <reg> <val>\r\n");
                out->ok = false;
                return true;
            }
            if (!execute) {
                snprintf(out->reply, sizeof(out->reply),
                         "OK smbus write 0x%02X 0x%02X 0x%02X\r\n", addr, reg, val);
                out->ok = true;
                return true;
            }
#ifndef OOB_HOST_TEST
            smbus_result_t r = smbus_write_byte(addr, reg, val);
            if (r == SMBUS_OK) {
                snprintf(out->reply, sizeof(out->reply),
                         "OK write 0x%02X 0x%02X = 0x%02X\r\n", addr, reg, val);
                out->ok = true;
            } else {
                snprintf(out->reply, sizeof(out->reply), "ERR write: %s\r\n", smbus_err_str(r));
                out->ok = false;
            }
            out->needs_smbus = true;
#else
            snprintf(out->reply, sizeof(out->reply),
                     "OK write 0x%02X 0x%02X = 0x%02X\r\n", addr, reg, val);
            out->ok = true;
#endif
            return true;
        }
        snprintf(out->reply, sizeof(out->reply), "ERR smbus subcommand\r\n");
        out->ok = false;
        return true;
    }

    if (match_cmd(&p, "reset")) {
        if (execute) {
            do_power(out, PIC_CMD_RESET, "reset", p);
        } else if (!has_confirm(p)) {
            snprintf(out->reply, sizeof(out->reply), "ERR reset requires CONFIRM\r\n");
            out->ok = false;
        } else {
            snprintf(out->reply, sizeof(out->reply), "OK reset\r\n");
            out->ok = true;
        }
        return true;
    }
    if (match_cmd(&p, "cycle")) {
        if (execute) {
            do_power(out, PIC_CMD_CYCLE, "cycle", p);
        } else if (!has_confirm(p)) {
            snprintf(out->reply, sizeof(out->reply), "ERR cycle requires CONFIRM\r\n");
            out->ok = false;
        } else {
            snprintf(out->reply, sizeof(out->reply), "OK cycle\r\n");
            out->ok = true;
        }
        return true;
    }
    if (match_cmd(&p, "off")) {
        if (execute) {
            do_power(out, PIC_CMD_OFF, "off", p);
        } else if (!has_confirm(p)) {
            snprintf(out->reply, sizeof(out->reply), "ERR off requires CONFIRM\r\n");
            out->ok = false;
        } else {
            snprintf(out->reply, sizeof(out->reply), "OK off\r\n");
            out->ok = true;
        }
        return true;
    }

    snprintf(out->reply, sizeof(out->reply), "ERR unknown command (try help)\r\n");
    out->ok = false;
    return true;
}

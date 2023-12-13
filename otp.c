#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/types.h>

#define countof(a) (sizeof(a) / sizeof((a)[0]))

#include "bcm28xx_otp.h"

extern void __iomem *otp_base;

static inline void otp_delay(void) {
  udelay(1);
}

static void otp_open(void) {
  iowrite32(3, otp_base + OTP_CONFIG_OFFSET);
  otp_delay();

  // Set OTP_CTRL_HI, OTP_CTRL_LO, OTP_ADDR, OTP_DATA to 0
  iowrite32(0, otp_base + OTP_CTRL_HI_OFFSET);
  iowrite32(0, otp_base + OTP_CTRL_LO_OFFSET);
  iowrite32(0, otp_base + OTP_ADDR_OFFSET);
  iowrite32(0, otp_base + OTP_DATA_OFFSET);

  // Set OTP_CONFIG to 2
  iowrite32(2, otp_base + OTP_CONFIG_OFFSET);
}

static void otp_close(void) {
  iowrite32(0, otp_base + OTP_CTRL_HI_OFFSET);
  iowrite32(0, otp_base + OTP_CTRL_LO_OFFSET);
  iowrite32(0, otp_base + OTP_CONFIG_OFFSET);
}

static int otp_wait_status(uint32_t mask, int retry) {
  int i = retry;
  do {
    otp_delay();
    if (ioread32(otp_base + OTP_STATUS_OFFSET) & mask) {
      return 0;
    }
  } while (--i);
  return -1;
}

static int otp_set_command(uint32_t ctrlhi, uint32_t cmd) {
  iowrite32(ctrlhi, otp_base + OTP_CTRL_HI_OFFSET);
  iowrite32(cmd << 1, otp_base + OTP_CTRL_LO_OFFSET);
  if (otp_wait_status(OTP_STAT_CMD_DONE, OTP_MAX_CMD_WAIT)) {
    return -1;
  }
  iowrite32((cmd << 1) | OTP_CMD_START, otp_base + OTP_CTRL_LO_OFFSET);
  return otp_wait_status(OTP_STAT_CMD_DONE, OTP_MAX_CMD_WAIT);
}

static uint32_t otp_read_open(uint8_t addr) {
  iowrite32(addr, otp_base + OTP_ADDR_OFFSET);
  return (uint32_t) otp_set_command(0, OTP_CMD_READ) ?: ioread32(otp_base + OTP_DATA_OFFSET);
}

static int otp_enable_program(void) {
  static const uint32_t seq[] = OTP_PROG_EN_SEQ;
  unsigned i;

  for (i = 0; i < countof(seq); ++i) {
#ifndef RPI4
    iowrite32(seq[i], otp_base + OTP_BITSEL_OFFSET);
#else
    iowrite32(seq[i], otp_base + OTP_DATA_OFFSET);
#endif
    if (otp_set_command(0, OTP_CMD_PROG_ENABLE))
      return -1;
  }
  return otp_wait_status(OTP_STAT_PROG_ENABLE, OTP_MAX_PROG_WAIT);
}

static int otp_disable_program(void) {
  return otp_set_command(0, OTP_CMD_PROG_DISABLE);
}

#ifndef RPI4
#error original LK code. needs to be migrated to linux kernel module format
static int otp_program_bit(uint8_t addr, uint8_t bit)
{
  uint32_t cmd;
  int err;
  int i;

  *REG32(OTP_ADDR) = addr;
  *REG32(OTP_BITSEL) = bit;

  if (addr < 8)
    cmd = 0x38000a;
  else if (addr < 77)
    cmd = 0x58000a;
  else
    cmd = 0x78000a;
  err = -1;
  for (i = 0; i < OTP_MAX_WRITE_RETRIES; ++i) {
    otp_set_command(0x1420000, cmd);
    if (!otp_set_command(0x1484, 0x88003) &&
        (*REG32(OTP_STATUS) & OTP_STAT_PROG_OK)) {
      err = 0;
      break;
    }
  }
  for (i = 0; i < OTP_MAX_WRITE_RETRIES; ++i) {
    otp_set_command(0x9420000, cmd);
    if (!otp_set_command(0x8001484, 0x88003) &&
        (*REG32(OTP_STATUS) & OTP_STAT_PROG_OK)) {
      err = 0;
      break;
    }
  }
  return err;
}

static int otp_write_enabled(uint8_t addr, uint32_t newval)
{
  int bitnum, err;
  uint32_t oldval = otp_read_open(addr);
  if (oldval == 0xffffffffL)    // This check guards against read failures
    return 0;

  err = 0;
  for (bitnum = 0; bitnum < 32; ++bitnum) {
    uint32_t bitmask = (uint32_t)1 << bitnum;
    if ((oldval & bitmask) != 0 || (newval & bitmask) == 0)
      continue;
    if (otp_program_bit(addr, bitnum) < 0)
      ++err;
  }
  return err;
}

#else // RPI4

static int otp_write_enabled(uint8_t addr, uint32_t newval) {
  uint32_t oldval = otp_read_open(addr);
  if (oldval == 0xffffffffL) {    // This check guards against read failures
    return 0;
  }
  iowrite32(newval | oldval, otp_base + OTP_DATA_OFFSET);
  otp_delay();
  iowrite32(addr, otp_base + OTP_ADDR_OFFSET);
  otp_delay();
  return otp_set_command(0, OTP_CMD_PROGRAM_WORD);
}

#endif // RPI4

static int otp_write_open(uint8_t addr, uint32_t val) {
  int err;
  if (otp_enable_program())
    return OTP_ERR_ENABLE;
  err = otp_write_enabled(addr, val);
  if (err)
    err = OTP_ERR_PROGRAM;
  if (otp_disable_program())
    err = err ?: OTP_ERR_DISABLE;
  return err;
}

// // //

uint32_t otp_read(uint8_t addr) {
  uint32_t val;
  otp_open();
  val = otp_read_open(addr);
  otp_close();
  return val;
}

int otp_write(uint8_t addr, uint32_t val) {
  int err;
  otp_open();
  err = otp_write_open(addr, val);
  otp_close();
  return err;
}
#pragma once
#include <linux/types.h>

#ifdef MMIO_BASE_VIRT
  #define BCM_PERIPH_BASE_VIRT    (MMIO_BASE_VIRT)
#elif BCM2711
  #define BCM_PERIPH_BASE_VIRT    (0xfe000000)
#elif BCM2837
  #define BCM_PERIPH_BASE_VIRT    (0xffffffffc0000000ULL)
  #define MEMORY_APERTURE_SIZE    (1024 * 1024 * 1024)
#elif ARCH_VPU
  #define BCM_PERIPH_BASE_VIRT    (0x7e000000U)
#else
  #error Unknown BCM28XX Variant
#endif

#define OTP_BASE_ADDR              (BCM_PERIPH_BASE_VIRT + 0x20f000)
#define OTP_LARGEST_OFFSET         (0x24) // this is to calculatue the memory range needed to map into process space
#define MEM_MAPPING_SIZE           (OTP_LARGEST_OFFSET + sizeof(uint32_t)) // see above. its for knowing how much memory to map

#define OTP_BOOTMODE_OFFSET        (0x00)
#define OTP_CONFIG_OFFSET          (0x04)
#define OTP_CTRL_LO_OFFSET         (0x08)
#define OTP_CTRL_HI_OFFSET         (0x0c)
#define OTP_STATUS_OFFSET          (0x10)
#define OTP_BITSEL_OFFSET          (0x14)
#define OTP_DATA_OFFSET            (0x18)
#define OTP_ADDR_OFFSET            (0x1c)
#define OTP_WRITE_DATA_READ_OFFSET (0x20)
#define OTP_INIT_STATUS_OFFSET     (0x24)

#define OTP_MAX_CMD_WAIT           (1000)
#define OTP_MAX_PROG_WAIT          (32000)
#define OTP_MAX_WRITE_RETRIES      (16)

#define OTP_ERR_PROGRAM            (-1)
#define OTP_ERR_ENABLE             (2)
#define OTP_ERR_DISABLE            (-3)

enum otp_command {
  OTP_CMD_READ = 0,
#ifndef RPI4
  OTP_CMD_PROG_ENABLE  = 0x1,
  OTP_CMD_PROG_DISABLE = 0x2,
#else
  OTP_CMD_PROG_ENABLE  = 0x2,
  OTP_CMD_PROG_DISABLE = 0x3,
#endif
  OTP_CMD_PROGRAM_WORD = 0xa,
};

/* OTP_CTRL_LO Bits */
#define OTP_CMD_START              (1)
#ifndef RPI4
#define OTP_STAT_PROG_OK           (4)
#define OTP_STAT_PROG_ENABLE       (0x1000)
#else
#define OTP_STAT_PROG_ENABLE       (4)
#endif

/* OTP Status Bits */
#ifdef RPI4
#define OTP_STAT_CMD_DONE          (2)
#else
#define OTP_STAT_CMD_DONE          (1)
#endif

#define OTP_PROG_EN_SEQ { 0xf, 0x4, 0x8, 0xd };

int otp_write(uint8_t addr, uint32_t val);
uint32_t otp_read(uint8_t addr);
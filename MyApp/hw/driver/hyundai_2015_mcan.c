/**
 * @file hyundai_2015_mcan.c
 *
 * @brief This source file was cleaned up to keep only cluster-related messages.
 */

#include <string.h>
#include "hyundai_2015_mcan.h"

static inline uint8_t pack_left_shift_u8(uint8_t value, uint8_t shift, uint8_t mask) { return (uint8_t)((uint8_t)(value << shift) & mask); }
static inline uint8_t pack_left_shift_u16(uint16_t value, uint8_t shift, uint8_t mask) { return (uint8_t)((uint8_t)(value << shift) & mask); }
static inline uint8_t pack_left_shift_u32(uint32_t value, uint8_t shift, uint8_t mask) { return (uint8_t)((uint8_t)(value << shift) & mask); }
static inline uint8_t pack_right_shift_u8(uint8_t value, uint8_t shift, uint8_t mask) { return (uint8_t)((uint8_t)(value >> shift) & mask); }
static inline uint8_t pack_right_shift_u32(uint32_t value, uint8_t shift, uint8_t mask) { return (uint8_t)((uint8_t)(value >> shift) & mask); }

static inline uint8_t unpack_left_shift_u32(uint8_t value, uint8_t shift, uint8_t mask) { return (uint32_t)((uint32_t)(value & mask) << shift); }
static inline uint8_t unpack_right_shift_u8(uint8_t value, uint8_t shift, uint8_t mask) { return (uint8_t)((uint8_t)(value & mask) >> shift); }
static inline uint32_t unpack_right_shift_u32(uint8_t value, uint8_t shift, uint8_t mask) { return (uint32_t)((uint32_t)(value & mask) >> shift); }

/* GW_CLU_P (0x56E) */
int hyundai_2015_mcan_gw_clu_p_pack(uint8_t *dst_p, const struct hyundai_2015_mcan_gw_clu_p_t *src_p, size_t size)
{
    if (size < 8u) return (-EINVAL);
    memset(&dst_p[0], 0, 8);
    dst_p[0] |= pack_left_shift_u8(src_p->c_vehicle_speed, 0u, 0xffu);
    dst_p[1] |= pack_left_shift_u32(src_p->c_odometer, 16u, 0xffu); // This might be wrong based on unpack, let's re-check unpack
    // Based on unpack:
    // dst_p->c_odometer = unpack_left_shift_u32(src_p[1], 16u, 0xffu);
    // dst_p->c_odometer |= unpack_left_shift_u32(src_p[2], 8u, 0xffu);
    // dst_p->c_odometer |= unpack_right_shift_u32(src_p[3], 0u, 0xffu);
    // So pack should be:
    dst_p[1] = (uint8_t)(src_p->c_odometer >> 16);
    dst_p[2] = (uint8_t)(src_p->c_odometer >> 8);
    dst_p[3] = (uint8_t)(src_p->c_odometer);
    return (8);
}

int hyundai_2015_mcan_gw_clu_p_unpack(struct hyundai_2015_mcan_gw_clu_p_t *dst_p, const uint8_t *src_p, size_t size)
{
    if (size < 8u) return (-EINVAL);
    dst_p->c_vehicle_speed = unpack_right_shift_u8(src_p[0], 0u, 0xffu);
    dst_p->c_odometer = ((uint32_t)src_p[1] << 16) | ((uint32_t)src_p[2] << 8) | (uint32_t)src_p[3];
    return (0);
}

int hyundai_2015_mcan_gw_clu_p_init(struct hyundai_2015_mcan_gw_clu_p_t *msg_p)
{
    if (msg_p == NULL) return -1;
    memset(msg_p, 0, sizeof(struct hyundai_2015_mcan_gw_clu_p_t));
    return 0;
}

/* HU_CLU_P_02 (0x508) */
int hyundai_2015_mcan_hu_clu_p_02_pack(uint8_t *dst_p, const struct hyundai_2015_mcan_hu_clu_p_02_t *src_p, size_t size)
{
    if (size < 8u) return (-EINVAL);
    memset(&dst_p[0], 0, 8);
    dst_p[0] |= pack_left_shift_u8(src_p->nv_time_type, 0u, 0x0fu);
    dst_p[1] |= pack_left_shift_u8(src_p->nv_hour, 0u, 0xffu);
    dst_p[2] |= pack_left_shift_u8(src_p->nv_min, 0u, 0xffu);
    return (8);
}

int hyundai_2015_mcan_hu_clu_p_02_unpack(struct hyundai_2015_mcan_hu_clu_p_02_t *dst_p, const uint8_t *src_p, size_t size)
{
    if (size < 8u) return (-EINVAL);
    dst_p->nv_time_type = unpack_right_shift_u8(src_p[0], 0u, 0x0fu);
    dst_p->nv_hour = unpack_right_shift_u8(src_p[1], 0u, 0xffu);
    dst_p->nv_min = unpack_right_shift_u8(src_p[2], 0u, 0xffu);
    return (0);
}

int hyundai_2015_mcan_hu_clu_p_02_init(struct hyundai_2015_mcan_hu_clu_p_02_t *msg_p)
{
    if (msg_p == NULL) return -1;
    memset(msg_p, 0, sizeof(struct hyundai_2015_mcan_hu_clu_p_02_t));
    return 0;
}

/* NM_CLU (0x44D) */
int hyundai_2015_mcan_nm_clu_pack(uint8_t *dst_p, const struct hyundai_2015_mcan_nm_clu_t *src_p, size_t size)
{
    if (size < 8u) return (-EINVAL);
    memset(&dst_p[0], 0, 8);
    dst_p[0] |= pack_left_shift_u8(src_p->destination_clu, 0u, 0xffu);
    dst_p[1] |= pack_left_shift_u8(src_p->nm_sleep_flag_clu, 4u, 0x30u);
    dst_p[1] |= pack_left_shift_u8(src_p->nm_command_code_clu, 0u, 0x07u);
    return (8);
}

int hyundai_2015_mcan_nm_clu_unpack(struct hyundai_2015_mcan_nm_clu_t *dst_p, const uint8_t *src_p, size_t size)
{
    if (size < 8u) return (-EINVAL);
    dst_p->destination_clu = unpack_right_shift_u8(src_p[0], 0u, 0xffu);
    dst_p->nm_sleep_flag_clu = unpack_right_shift_u8(src_p[1], 4u, 0x30u);
    dst_p->nm_command_code_clu = unpack_right_shift_u8(src_p[1], 0u, 0x07u);
    return (0);
}

int hyundai_2015_mcan_nm_clu_init(struct hyundai_2015_mcan_nm_clu_t *msg_p)
{
    if (msg_p == NULL) return -1;
    memset(msg_p, 0, sizeof(struct hyundai_2015_mcan_nm_clu_t));
    return 0;
}

/**
 * [SAMPLE] Cluster related message usage sample.
 */
void hyundai_2015_mcan_sample_cluster(void)
{
  // 1. Unpack Example (GW_CLU_P - 0x56E)
  uint8_t rx_data[8] = {0x64, 0x01, 0x02, 0x03, 0, 0, 0, 0}; 
  struct hyundai_2015_mcan_gw_clu_p_t clu_p;
  
  if (hyundai_2015_mcan_gw_clu_p_unpack(&clu_p, rx_data, 8) == 0)
  {
    (void)clu_p.c_vehicle_speed;
    (void)clu_p.c_odometer;
  }

  // 2. Pack Example (HU_CLU_P_02 - 0x508)
  struct hyundai_2015_mcan_hu_clu_p_02_t hu_clu_p2;
  uint8_t tx_buffer[8];
  
  hyundai_2015_mcan_hu_clu_p_02_init(&hu_clu_p2);
  hu_clu_p2.nv_hour = 12;
  hu_clu_p2.nv_min = 30;
  hu_clu_p2.nv_time_type = 1;
  
  int packed_size = hyundai_2015_mcan_hu_clu_p_02_pack(tx_buffer, &hu_clu_p2, 8);
  if (packed_size > 0)
  {
    (void)tx_buffer;
  }
}

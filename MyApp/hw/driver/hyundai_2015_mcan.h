/**
 * @file hyundai_2015_mcan.h
 *
 * @brief This header file was cleaned up to keep only cluster-related messages.
 */

#ifndef HYUNDAI_2015_MCAN_H
#define HYUNDAI_2015_MCAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef EINVAL
#define EINVAL 22
#endif

/* Frame ids. */

#define HYUNDAI_2015_MCAN_GW_CLU_P_FRAME_ID (0x56eu)
#define HYUNDAI_2015_MCAN_HU_CLU_P_02_FRAME_ID (0x508u)
#define HYUNDAI_2015_MCAN_NM_CLU_FRAME_ID (0x44du)

/* Frame lengths in bytes. */
#define HYUNDAI_2015_MCAN_GW_CLU_P_LENGTH (8u)
#define HYUNDAI_2015_MCAN_HU_CLU_P_02_LENGTH (8u)
#define HYUNDAI_2015_MCAN_NM_CLU_LENGTH (8u)

/**
 * Signals in message GW_CLU_P.
 */
struct hyundai_2015_mcan_gw_clu_p_t {
    uint8_t c_vehicle_speed;
    uint32_t c_odometer;
};

/**
 * Signals in message HU_CLU_P_02.
 */
struct hyundai_2015_mcan_hu_clu_p_02_t {
    uint8_t nv_time_type;
    uint8_t nv_hour;
    uint8_t nv_min;
};

/**
 * Signals in message NM_CLU.
 */
struct hyundai_2015_mcan_nm_clu_t {
    uint8_t destination_clu;
    uint8_t nm_sleep_flag_clu;
    uint8_t nm_command_code_clu;
};

/* GW_CLU_P */
int hyundai_2015_mcan_gw_clu_p_pack(uint8_t *dst_p, const struct hyundai_2015_mcan_gw_clu_p_t *src_p, size_t size);
int hyundai_2015_mcan_gw_clu_p_unpack(struct hyundai_2015_mcan_gw_clu_p_t *dst_p, const uint8_t *src_p, size_t size);
int hyundai_2015_mcan_gw_clu_p_init(struct hyundai_2015_mcan_gw_clu_p_t *msg_p);

/* HU_CLU_P_02 */
int hyundai_2015_mcan_hu_clu_p_02_pack(uint8_t *dst_p, const struct hyundai_2015_mcan_hu_clu_p_02_t *src_p, size_t size);
int hyundai_2015_mcan_hu_clu_p_02_unpack(struct hyundai_2015_mcan_hu_clu_p_02_t *dst_p, const uint8_t *src_p, size_t size);
int hyundai_2015_mcan_hu_clu_p_02_init(struct hyundai_2015_mcan_hu_clu_p_02_t *msg_p);

/* NM_CLU */
int hyundai_2015_mcan_nm_clu_pack(uint8_t *dst_p, const struct hyundai_2015_mcan_nm_clu_t *src_p, size_t size);
int hyundai_2015_mcan_nm_clu_unpack(struct hyundai_2015_mcan_nm_clu_t *dst_p, const uint8_t *src_p, size_t size);
int hyundai_2015_mcan_nm_clu_init(struct hyundai_2015_mcan_nm_clu_t *msg_p);

/**
 * [SAMPLE] Cluster related message usage sample.
 */
void hyundai_2015_mcan_sample_cluster(void);

#ifdef __cplusplus
}
#endif

#endif

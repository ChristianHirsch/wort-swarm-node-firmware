#ifndef UUID_H_
#define UUID_H_

#include <bluetooth/uuid.h>

/**
 * UUID (v4): 8feeXXXX-3c17-4189-8556-a293fa6b2739
 */

#define BT_UUID_BASE		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee0000, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_ASS
 *  @brief AFarCloud Synchronization Service
 */
#define BT_UUID_ASS			BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee1801, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_COS_INPUT_IO
 *  @brief ASS Generic Synchronization Characteristics
 */
#define BT_UUID_ASS_GSC		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee2a01, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_COS_INPUT_POWER
 *  @brief ASS Timestamp Descriptor
 */
#define BT_UUID_ASS_TID		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee2901, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_COS_OUTPUT_IO
 *  @brief ASS Ambient Temperature Descriptor
 */
#define BT_UUID_ASS_ATD		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee2902, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_COS_OUTPUT_POWER
 *  @brief ASS Relative Humidity Descriptor
 */
#define BT_UUID_ASS_RHD		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee2903, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_ASS_APR
 *  @brief ASS Atmospheric Pressure Descriptor
 */
#define BT_UUID_ASS_APR		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee2916, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_ASS_BVD
 *  @brief ASS Battery Voltage Descriptor
 */
#define BT_UUID_ASS_BVD		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee2919, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_ASS_ACX
 *  @brief ASS Accelerator X Descriptor
 */
#define BT_UUID_ASS_ACX		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee291a, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_ASS_ACY
 *  @brief ASS Accelerator Y Descriptor
 */
#define BT_UUID_ASS_ACY		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee291b, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_ASS_ACX
 *  @brief ASS Accelerator Z Descriptor
 */
#define BT_UUID_ASS_ACZ		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee291c, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_FUS
 *  @brief FUS Firmware Update Service
 */
#define BT_UUID_FUS		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee1805, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

/** @def BT_UUID_ASS_BVD
 *  @brief ASS Battery Voltage Descriptor
 */
#define BT_UUID_FUS_FIMWARE		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8fee2ab1, 0x3c17, 0x4189, 0x8556, 0xa293fa6b2739))

#endif /* UUID_H_ */

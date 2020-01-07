/** @file
 *  @brief BAS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

void bas_init(void);
int  bas_notify(u16_t _battery_lvl);

#ifdef __cplusplus
}
#endif

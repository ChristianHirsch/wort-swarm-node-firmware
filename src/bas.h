/** @file
 *  @brief BAS Service sample
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void bas_init(void);
int  bas_notify(uint16_t _battery_lvl);

#ifdef __cplusplus
}
#endif

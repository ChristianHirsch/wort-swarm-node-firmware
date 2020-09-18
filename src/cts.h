#ifndef CTS_H_
#define CTS_H_

#ifdef __cplusplus
extern "C" {
#endif

void cts_init(void);
int  cts_notify(u32_t _timestamp);

#ifdef __cplusplus
}
#endif

#endif /* CTS_H_ */


#ifndef _IMX_SNVS_H_
#define _IMX_SNVS_H_
#include <trusty/trusty_ipc.h>

uint32_t trusty_snvs_read(uint32_t target);
void trusty_snvs_write(uint32_t target, uint32_t value);
void trusty_snvs_update_lpcr(uint32_t target, uint32_t enable);
int imx_snvs_init(struct trusty_ipc_dev *dev);

#endif

#include <trusty/trusty_ipc.h>
#include <trusty/util.h>
#include <trusty/imx_snvs.h>
#include <trusty/trusty_dev.h>
#include "arch/arm/smcall.h"

#define SMC_ENTITY_SNVS_RTC 53
#define SMC_SNVS_PROBE SMC_FASTCALL_NR(SMC_ENTITY_SNVS_RTC, 0)
#define SMC_SNVS_REGS_OP SMC_FASTCALL_NR(SMC_ENTITY_SNVS_RTC, 1)
#define SMC_SNVS_LPCR_OP SMC_FASTCALL_NR(SMC_ENTITY_SNVS_RTC, 2)

#define OPT_READ 0x1
#define OPT_WRITE 0x2

static struct trusty_ipc_dev *_dev = NULL;

uint32_t trusty_snvs_read(uint32_t target) {
    if (!_dev) {
        trusty_error("trusty imx snvs driver is not initialized!\n");
	return 0;
    }
    return trusty_simple_fast_call32(SMC_SNVS_REGS_OP, target, OPT_READ, 0);
}

void trusty_snvs_write(uint32_t target, uint32_t value) {
    if (!_dev) {
        trusty_error("trusty imx snvs driver is not initialized!\n");
	return;
    }
    trusty_simple_fast_call32(SMC_SNVS_REGS_OP, target, OPT_WRITE, value);
}

void trusty_snvs_update_lpcr(uint32_t target, uint32_t enable) {
    if (!_dev) {
        trusty_error("trusty imx snvs driver is not initialized!\n");
	return;
    }
    trusty_simple_fast_call32(SMC_SNVS_LPCR_OP, target, enable, 0);
}

int imx_snvs_init(struct trusty_ipc_dev *dev)
{
    trusty_assert(dev);
    int error;
    error = trusty_simple_fast_call32(SMC_SNVS_PROBE, 0, 0, 0);
    if (error < 0) {
        trusty_error("trusty imx snvs driver initialize failed! error=%d\n", error);
        return error;
    }
    _dev = dev;
    return 0;

}

/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _SCFW_UTILS_H_
#define _SCFW_UTILS_H_

#include <common.h>
#include <asm/arch/sci/sci.h>

static int g_debug_scfw;	/* set to one to turn on SCFW API tracing */

#define SC_PM_SET_CLOCK_PARENT(__ipcHndl__, __res__, __clk__, __parent__) \
do { \
	char _res_str[]	= #__res__;\
	char _clk_str[]	= #__clk__;\
	sc_err_t _ret;\
	if (g_debug_scfw) \
		printf("(%4d) sc_pm_set_clock_parent %s:%s -> %d\n",\
		       __LINE__, _res_str, _clk_str, __parent__);\
	_ret = sc_pm_set_clock_parent(__ipcHndl__,\
		__res__, __clk__, __parent__);\
	if (_ret != SC_ERR_NONE) \
		printf("(%d)>> sc_pm_set_clock_parent failed! %s:%s -> %d (error = %d)\n",\
		       __LINE__, _res_str, _clk_str, __parent__, _ret);\
} while (0)

#define SC_PM_SET_CLOCK_RATE(__ipcHndl__, __res__, __clk__, __rate__) \
do { \
	char _res_str[]	= #__res__;\
	char _clk_str[]	= #__clk__;\
	sc_err_t _ret;\
	sc_pm_clock_rate_t _actual = __rate__;\
	if (g_debug_scfw) \
		printf("(%4d) sc_pm_set_clock_rate %s:%s -> %d\n",\
		       __LINE__, _res_str, _clk_str,  __rate__);\
	_ret = sc_pm_set_clock_rate(__ipcHndl__, __res__, __clk__, &_actual);\
	if (_ret != SC_ERR_NONE)\
		printf("(%4d)>> sc_pm_set_clock_rate failed! %s:%s -> %d (error = %d)\n",\
		       __LINE__, _res_str, _clk_str, __rate__, _ret);\
	if (_actual != __rate__)\
		printf("(%4d)>> Actual rate for %s:%s is %d instead of %d\n", \
		       __LINE__, _res_str, _clk_str, _actual, __rate__);  \
} while (0)

#define SC_PM_CLOCK_ENABLE(__ipcHndl__, __res__, __clk__, __enable__) \
do { \
	char _res_str[]	= #__res__;\
	char _clk_str[]	= #__clk__;\
	sc_err_t _ret;\
	if (g_debug_scfw) \
		printf("(%4d) sc_pm_clock_enable %s:%s -> %d\n",\
		       __LINE__, _res_str, _clk_str, __enable__);\
	_ret = sc_pm_clock_enable(__ipcHndl__,\
				 __res__, __clk__, __enable__, false);\
	if (_ret != SC_ERR_NONE)\
		printf("(%4d)>> sc_pm_clock_enable failed! %s:%s -> %d (error = %d)\n",\
			__LINE__, _res_str, _clk_str, __enable__, _ret);\
} while (0) \

#define SC_MISC_SET_CONTROL(__ipcHndl__, __res__, __clk__, __value__) \
do { \
	char _res_str[]	= #__res__; \
	char _clk_str[]	= #__clk__; \
	sc_err_t _ret; \
	if (g_debug_scfw) \
			printf("(%4d) sc_misc_set_control %s:%s -> %d\n",\
			       __LINE__, _res_str, _clk_str, __value__);\
	_ret = sc_misc_set_control(__ipcHndl__, \
				 __res__, __clk__, __value__); \
	if (_ret != SC_ERR_NONE) \
		printf("(%4d)>> sc_misc_set_control failed! %s:%s -> %d (error = %d)\n", \
			__LINE__, _res_str, _clk_str, __value__, _ret); \
} while (0)

#define SC_PM_SET_RESOURCE_POWER_MODE(__ipcHndl__, __res__, __enable__) \
do { \
	char _res_str[]	= #__res__; \
	sc_err_t _ret; \
	if (g_debug_scfw) \
			printf("(%4d) sc_pm_set_resource_power_mode %s -> %d\n",\
			       __LINE__, _res_str, __enable__);\
	_ret = sc_pm_set_resource_power_mode(__ipcHndl__, __res__, __enable__);\
	if (_ret != SC_ERR_NONE) \
		printf("(%4d)>> sc_pm_set_resource_power_mode failed! %s -> %d (error = %d)\n", \
		       __LINE__, _res_str, __enable__, _ret);\
} while (0)

#define SC_MISC_AUTH(__ipcHndl__, __cmd__, __addr__) \
do { \
	sc_err_t _ret; \
	if (g_debug_scfw) \
			printf("(%4d) sc_misc_seco_authenticate ->  cmd %d addr %d\n",\
			       __LINE__, __cmd__, __addr__);\
	_ret = sc_seco_authenticate(__ipcHndl__, __cmd__, __addr__); \
	if (_ret != SC_ERR_NONE) \
		printf("(%4d)>> sc_misc_seco_authenticate cmd %d addr %d (error = %d)\n", \
			__LINE__, __cmd__, __addr__, _ret); \
} while (0)

#endif /*_SCFW_UTILS_H_ */

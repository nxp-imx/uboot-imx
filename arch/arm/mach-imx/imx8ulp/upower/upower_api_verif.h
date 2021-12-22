/* SPDX-License-Identifier: BSD-3-Clause */
/* +FHDR------------------------------------------------------------------------
 * Copyright 2019-2021 NXP
 * -----------------------------------------------------------------------------
 * FILE NAME      : upower_api_verif.h
 * DEPARTMENT     : BSTC - Campinas, Brazil
 * AUTHOR         : Celso Brites
 * AUTHOR'S EMAIL : celso.brites@nxp.com
 * -----------------------------------------------------------------------------
 * RELEASE HISTORY
 * VERSION DATE        AUTHOR                  DESCRIPTION
 *
 * $Log: upower_api_verif.h.rca $
 *
 *  Revision: 1.4 Tue Sep 15 17:52:21 2020 nxa11511
 *  Verification-only service upwr_pwm_reg_access moved to Exception service group and was renamed upwr_xcp_reg_access.
 *
 *  Revision: 1.3 Wed Jun 17 17:01:08 2020 nxa11511
 *  Comment fix.
 *
 *  Revision: 1.2 Mon Jun  8 06:46:13 2020 nxa11511
 *  Removes verification-only typedefs, moved to upower_soc_defs.h
 *
 * -----------------------------------------------------------------------------
 * KEYWORDS: micro-power uPower driver API verification
 * -----------------------------------------------------------------------------
 * PURPOSE: uPower driver verification-only API:
 *          calls available only for veriifcation, but not production
 * -----------------------------------------------------------------------------
 * PARAMETERS:
 * PARAM NAME RANGE:DESCRIPTION:       DEFAULTS:                           UNITS
 * -----------------------------------------------------------------------------
 * REUSE ISSUES: no reuse issues
 * -FHDR------------------------------------------------------------------------
 */

#ifndef _UPWR_API_VERIF_H_
#define _UPWR_API_VERIF_H_

/* definitions for the verification-only service upwr_pwm_read_write */

/*
 * upwr_xcp_reg_access()- accesses (read or write) a register inside uPower.
 * @access: pointer to the access specification struct (see upower_soc_defs.h).
 *          access->mask determines the bits of access->data written (if any)
 *          at access->addr; if access->mask = 0, the service performs a read.
 * @callb: pointer to the callback called when configurations are applied.
 * NULL if no callback is required.
 *
 * A callback can be optionally registered, and will be called upon the arrival
 * of the request response from the uPower firmware, telling if it succeeded
 * or not.
 *
 * A callback may not be registered (NULL pointer), in which case polling has
 * to be used to check a service group response, by calling upwr_req_status or
 * upwr_poll_req_status, using UPWR_SG_PWRMGMT as the service group argument.
 *
 * This service has as return value the final register value (read value on a
 * read or the updated value on a write), which is obtained from the callback
 * argument ret or *retptr in case of polling using the functions
 * upwr_req_status or upwr_poll_req_status.
 *
 * Context: no sleep, no locks taken/released.
 * Return: 0 if ok, -1 if service group is busy,
 *        -2 if the pointer conversion to physical address failed,
 *        -3 if called in an invalid API state.
 * Note that this is not the error response from the request itself:
 * it only tells if the request was successfully sent to the uPower.
 */

int upwr_xcp_reg_access(const struct upwr_reg_access_t* access,
			upwr_callb                      callb);
#endif

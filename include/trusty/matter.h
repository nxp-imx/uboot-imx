/*
 * Copyright 2022 NXP
 */

#ifndef TRUSTY_MATTER_H_
#define TRUSTY_MATTER_H_

#include <trusty/sysdeps.h>
#include <trusty/trusty_ipc.h>
#include <interface/matter/matter.h>

/*
 * Initialize Matter TIPC client. Returns one of trusty_err.
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
int matter_tipc_init(struct trusty_ipc_dev *dev);

/*
 * Shutdown Matter TIPC client.
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
void matter_tipc_shutdown(struct trusty_ipc_dev *dev);

/*
 * set matter DAC (Device Attestation Certificate).
 *
 * @ cert: pointer to certificate data
 * @ cert_size: certificate size
 */
int trusty_set_dac_cert(const uint8_t *cert, uint32_t cert_size);

/*
 * set matter PAI (Product Attestation Intermediate) Certificate.
 *
 * @ cert: pointer to certificate data
 * @ cert_size: certificate size
 */
int trusty_set_pai_cert(const uint8_t *cert, uint32_t cert_size);

/*
 * set matter CD (Certification Declaration).
 *
 * @ cert: pointer to certificate data
 * @ cert_size: certificate size
 */
int trusty_set_cd_cert(const uint8_t *cert, uint32_t cert_size);

/*
 * set matter DAC private key.
 *
 * @ key: pointer to DAC private key
 * @ key_size: key size
 */
int trusty_set_dac_prikey(const uint8_t *key, uint32_t key_size);
#endif /* TRUSTY_MATTER_H_ */

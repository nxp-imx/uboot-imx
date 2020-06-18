/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TRUSTY_KEYMASTER_H_
#define TRUSTY_KEYMASTER_H_

#include <trusty/sysdeps.h>
#include <trusty/trusty_ipc.h>
#include <interface/keymaster/keymaster.h>

/*
 * Initialize Keymaster TIPC client. Returns one of trusty_err.
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
int km_tipc_init(struct trusty_ipc_dev *dev);

/*
 * Shutdown Keymaster TIPC client.
 *
 * @dev: initialized with trusty_ipc_dev_create
 */
void km_tipc_shutdown(struct trusty_ipc_dev *dev);

/*
 * Set Keymaster boot parameters. Returns one of trusty_err.
 *
 * @os_version: OS version from Android image header
 * @os_patchlevel: OS patch level from Android image header
 * @verified_boot_state: one of keymaster_verified_boot_t
 * @device_locked: nonzero if device is locked
 * @verified_boot_key_hash: hash of key used to verify Android image
 * @verified_boot_key_hash_size: size of verified_boot_key_hash
 * @verified_boot_hash: cumulative hash of all images verified thus far.
 *                      May be NULL if not computed.
 * @verified_boot_hash_size: size of verified_boot_hash
 */
int trusty_set_boot_params(uint32_t os_version, uint32_t os_patchlevel,
                           keymaster_verified_boot_t verified_boot_state,
                           bool device_locked,
                           const uint8_t *verified_boot_key_hash,
                           uint32_t verified_boot_key_hash_size,
                           const uint8_t *verified_boot_hash,
                           uint32_t verified_boot_hash_size);

/*
 * Set Keymaster attestation key. Returns one of trusty_err.
 *
 * @key: buffer containing key
 * @key_size: size of key in bytes
 * @algorithm: one of KM_ALGORITHM_RSA or KM_ALGORITHM_EC
 */
int trusty_set_attestation_key(const uint8_t *key, uint32_t key_size,
                               keymaster_algorithm_t algorithm);

/*
 * Append certificate to Keymaster attestation certificate chain. Returns
 * one of trusty_err.
 *
 * @cert: buffer containing certificate
 * @cert_size: size of certificate in bytes
 * @algorithm: one of KM_ALGORITHM_RSA or KM_ALGORITHM_EC
 */
int trusty_append_attestation_cert_chain(const uint8_t *cert,
                                         uint32_t cert_size,
                                         keymaster_algorithm_t algorithm);
/*
 * Set encrypted Keymaster attestation key. Returns one of trusty_err.
 *
 * @key: buffer containing encrypted key
 * @key_size: size of key in bytes
 * @algorithm: one of KM_ALGORITHM_RSA or KM_ALGORITHM_EC
 */
int trusty_set_attestation_key_enc(const uint8_t *key,
                                   uint32_t key_size,
                                   keymaster_algorithm_t algorithm);

/*
 * Append encrypted certificate to Keymaster attestation certificate chain. Returns
 * one of trusty_err.
 *
 * @cert: buffer containing encrypted certificate
 * @cert_size: size of certificate in bytes
 * @algorithm: one of KM_ALGORITHM_RSA or KM_ALGORITHM_EC
 */
int trusty_append_attestation_cert_chain_enc(const uint8_t *cert,
                                             uint32_t cert_size,
                                             keymaster_algorithm_t algorithm);
/*
 * Reads a CA Request from Keymaster. On success allocates a new CA Request
 * message at |*ca_request_p|, and the caller takes ownership. Returns one
 * of trusty_err.
 *
 * @operation_start: Operation Start message
 * @operation_start_size: size of operation_start
 * @ca_request_p: location of newly allocated CA Request message
 * @ca_request_size_p: location of size of the CA Request message
 */
int trusty_atap_get_ca_request(const uint8_t *operation_start,
                               uint32_t operation_start_size,
                               uint8_t** ca_request_p,
                               uint32_t* ca_request_size_p);
/*
 * Sends the CA Response to Keymaster. Returns one of trusty_err.
 *
 * @ca_response: CA Response message
 * @ca_response_size: size of ca_response
 */
int trusty_atap_set_ca_response(const uint8_t *ca_response,
                                uint32_t ca_response_size);

/*
* Reads the UUID from the certificate of the last provisioned attestation
* credentials as a c-string into |*uuid_p|. Caller takes ownership of
* |*uuid_p|. Returns one of trusty_err.
*
* @uuid_p: location of newly allocated UUID c-string
*/
int trusty_atap_read_uuid_str(char **uuid_p);

/*
 * SetProductId is only called once to set the secure product id. Caller should
 * read the product id from permanent attributes structure and set the product
 * id while fusing the permanent attributes.
 *
 * @product_id: The product id to be set.
 * @size: The size of the product id.
 */
int trusty_set_product_id(const uint8_t *product_id, uint32_t size);

/*
 * trusty_get_mppubk is called to get the mppubk from trusty side.
 *
 * @mppubk: Pointer to the buffer which store the mppubk.
 * @size:  Pointer to The size of mppubk.
 */
int trusty_get_mppubk(uint8_t *mppubk, uint32_t* size);

/* trusty_verify_secure_unlock is called to the verify the secure unlock
 * credential.
 *
 * @unlock_credential: Poniter to the unlock credential.
 * @credential_size: credential size.
 * @serial: serial number to verify.
 * @serial_size: serial number size.
 */
int trusty_verify_secure_unlock(uint8_t *unlock_credential,
                                uint32_t credential_size,
                                uint8_t *serial, uint32_t serial_size);

#endif /* TRUSTY_KEYMASTER_H_ */

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

#ifndef TRUSTY_INTERFACE_KEYMASTER_H_
#define TRUSTY_INTERFACE_KEYMASTER_H_

#include <trusty/sysdeps.h>

#define KEYMASTER_PORT "com.android.trusty.keymaster"
#define KEYMASTER_MAX_BUFFER_LENGTH 4096

enum keymaster_command {
    KEYMASTER_RESP_BIT              = 1,
    KEYMASTER_STOP_BIT              = 2,
    KEYMASTER_REQ_SHIFT             = 2,

    KM_GENERATE_KEY                 = (0 << KEYMASTER_REQ_SHIFT),
    KM_BEGIN_OPERATION              = (1 << KEYMASTER_REQ_SHIFT),
    KM_UPDATE_OPERATION             = (2 << KEYMASTER_REQ_SHIFT),
    KM_FINISH_OPERATION             = (3 << KEYMASTER_REQ_SHIFT),
    KM_ABORT_OPERATION              = (4 << KEYMASTER_REQ_SHIFT),
    KM_IMPORT_KEY                   = (5 << KEYMASTER_REQ_SHIFT),

    KM_EXPORT_KEY                   = (6 << KEYMASTER_REQ_SHIFT),
    KM_GET_VERSION                  = (7 << KEYMASTER_REQ_SHIFT),
    KM_ADD_RNG_ENTROPY              = (8 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_ALGORITHMS     = (9 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_BLOCK_MODES    = (10 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_PADDING_MODES  = (11 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_DIGESTS        = (12 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_IMPORT_FORMATS = (13 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_EXPORT_FORMATS = (14 << KEYMASTER_REQ_SHIFT),
    KM_GET_KEY_CHARACTERISTICS      = (15 << KEYMASTER_REQ_SHIFT),

    // Bootloader calls.
    KM_SET_BOOT_PARAMS                 = (0x1000 << KEYMASTER_REQ_SHIFT),
    KM_SET_ATTESTATION_KEY             = (0x2000 << KEYMASTER_REQ_SHIFT),
    KM_APPEND_ATTESTATION_CERT_CHAIN   = (0x3000 << KEYMASTER_REQ_SHIFT),
    KM_ATAP_GET_CA_REQUEST             = (0x4000 << KEYMASTER_REQ_SHIFT),
    KM_ATAP_SET_CA_RESPONSE_BEGIN      = (0x5000 << KEYMASTER_REQ_SHIFT),
    KM_ATAP_SET_CA_RESPONSE_UPDATE     = (0x6000 << KEYMASTER_REQ_SHIFT),
    KM_ATAP_SET_CA_RESPONSE_FINISH     = (0x7000 << KEYMASTER_REQ_SHIFT),
    KM_ATAP_READ_UUID                  = (0x8000 << KEYMASTER_REQ_SHIFT),
    KM_SET_PRODUCT_ID                  = (0x9000 << KEYMASTER_REQ_SHIFT),
    KM_SET_ATTESTATION_KEY_ENC         = (0xa000 << KEYMASTER_REQ_SHIFT),
    KM_APPEND_ATTESTATION_CERT_CHAIN_ENC = (0xb000 << KEYMASTER_REQ_SHIFT),
    KM_GET_MPPUBK                      = (0xc000 << KEYMASTER_REQ_SHIFT),
    KM_VERIFY_SECURE_UNLOCK            = (0xd000 << KEYMASTER_REQ_SHIFT)
};

typedef enum {
    KM_VERIFIED_BOOT_VERIFIED = 0,    /* Full chain of trust extending from the bootloader to
                                       * verified partitions, including the bootloader, boot
                                       * partition, and all verified partitions*/
    KM_VERIFIED_BOOT_SELF_SIGNED = 1, /* The boot partition has been verified using the embedded
                                       * certificate, and the signature is valid. The bootloader
                                       * displays a warning and the fingerprint of the public
                                       * key before allowing the boot process to continue.*/
    KM_VERIFIED_BOOT_UNVERIFIED = 2,  /* The device may be freely modified. Device integrity is left
                                       * to the user to verify out-of-band. The bootloader
                                       * displays a warning to the user before allowing the boot
                                       * process to continue */
    KM_VERIFIED_BOOT_FAILED = 3,      /* The device failed verification. The bootloader displays a
                                       * warning and stops the boot process, so no keymaster
                                       * implementation should ever actually return this value,
                                       * since it should not run.  Included here only for
                                       * completeness. */
} keymaster_verified_boot_t;

/**
 * Algorithms that may be provided by keymaster implementations.
 */
typedef enum {
    /* Asymmetric algorithms. */
    KM_ALGORITHM_RSA = 1,
    // KM_ALGORITHM_DSA = 2, -- Removed, do not re-use value 2.
    KM_ALGORITHM_EC = 3,

    /* Block ciphers algorithms */
    KM_ALGORITHM_AES = 32,

    /* MAC algorithms */
    KM_ALGORITHM_HMAC = 128,
} keymaster_algorithm_t;

typedef enum {
    KM_ERROR_OK = 0,
    KM_ERROR_ROOT_OF_TRUST_ALREADY_SET = -1,
    KM_ERROR_UNSUPPORTED_PURPOSE = -2,
    KM_ERROR_INCOMPATIBLE_PURPOSE = -3,
    KM_ERROR_UNSUPPORTED_ALGORITHM = -4,
    KM_ERROR_INCOMPATIBLE_ALGORITHM = -5,
    KM_ERROR_UNSUPPORTED_KEY_SIZE = -6,
    KM_ERROR_UNSUPPORTED_BLOCK_MODE = -7,
    KM_ERROR_INCOMPATIBLE_BLOCK_MODE = -8,
    KM_ERROR_UNSUPPORTED_MAC_LENGTH = -9,
    KM_ERROR_UNSUPPORTED_PADDING_MODE = -10,
    KM_ERROR_INCOMPATIBLE_PADDING_MODE = -11,
    KM_ERROR_UNSUPPORTED_DIGEST = -12,
    KM_ERROR_INCOMPATIBLE_DIGEST = -13,
    KM_ERROR_INVALID_EXPIRATION_TIME = -14,
    KM_ERROR_INVALID_USER_ID = -15,
    KM_ERROR_INVALID_AUTHORIZATION_TIMEOUT = -16,
    KM_ERROR_UNSUPPORTED_KEY_FORMAT = -17,
    KM_ERROR_INCOMPATIBLE_KEY_FORMAT = -18,
    KM_ERROR_UNSUPPORTED_KEY_ENCRYPTION_ALGORITHM = -19,   /* For PKCS8 & PKCS12 */
    KM_ERROR_UNSUPPORTED_KEY_VERIFICATION_ALGORITHM = -20, /* For PKCS8 & PKCS12 */
    KM_ERROR_INVALID_INPUT_LENGTH = -21,
    KM_ERROR_KEY_EXPORT_OPTIONS_INVALID = -22,
    KM_ERROR_DELEGATION_NOT_ALLOWED = -23,
    KM_ERROR_KEY_NOT_YET_VALID = -24,
    KM_ERROR_KEY_EXPIRED = -25,
    KM_ERROR_KEY_USER_NOT_AUTHENTICATED = -26,
    KM_ERROR_OUTPUT_PARAMETER_NULL = -27,
    KM_ERROR_INVALID_OPERATION_HANDLE = -28,
    KM_ERROR_INSUFFICIENT_BUFFER_SPACE = -29,
    KM_ERROR_VERIFICATION_FAILED = -30,
    KM_ERROR_TOO_MANY_OPERATIONS = -31,
    KM_ERROR_UNEXPECTED_NULL_POINTER = -32,
    KM_ERROR_INVALID_KEY_BLOB = -33,
    KM_ERROR_IMPORTED_KEY_NOT_ENCRYPTED = -34,
    KM_ERROR_IMPORTED_KEY_DECRYPTION_FAILED = -35,
    KM_ERROR_IMPORTED_KEY_NOT_SIGNED = -36,
    KM_ERROR_IMPORTED_KEY_VERIFICATION_FAILED = -37,
    KM_ERROR_INVALID_ARGUMENT = -38,
    KM_ERROR_UNSUPPORTED_TAG = -39,
    KM_ERROR_INVALID_TAG = -40,
    KM_ERROR_MEMORY_ALLOCATION_FAILED = -41,
    KM_ERROR_IMPORT_PARAMETER_MISMATCH = -44,
    KM_ERROR_SECURE_HW_ACCESS_DENIED = -45,
    KM_ERROR_OPERATION_CANCELLED = -46,
    KM_ERROR_CONCURRENT_ACCESS_CONFLICT = -47,
    KM_ERROR_SECURE_HW_BUSY = -48,
    KM_ERROR_SECURE_HW_COMMUNICATION_FAILED = -49,
    KM_ERROR_UNSUPPORTED_EC_FIELD = -50,
    KM_ERROR_MISSING_NONCE = -51,
    KM_ERROR_INVALID_NONCE = -52,
    KM_ERROR_MISSING_MAC_LENGTH = -53,
    KM_ERROR_KEY_RATE_LIMIT_EXCEEDED = -54,
    KM_ERROR_CALLER_NONCE_PROHIBITED = -55,
    KM_ERROR_KEY_MAX_OPS_EXCEEDED = -56,
    KM_ERROR_INVALID_MAC_LENGTH = -57,
    KM_ERROR_MISSING_MIN_MAC_LENGTH = -58,
    KM_ERROR_UNSUPPORTED_MIN_MAC_LENGTH = -59,
    KM_ERROR_UNSUPPORTED_KDF = -60,
    KM_ERROR_UNSUPPORTED_EC_CURVE = -61,
    KM_ERROR_KEY_REQUIRES_UPGRADE = -62,
    KM_ERROR_ATTESTATION_CHALLENGE_MISSING = -63,
    KM_ERROR_KEYMASTER_NOT_CONFIGURED = -64,

    KM_ERROR_UNIMPLEMENTED = -100,
    KM_ERROR_VERSION_MISMATCH = -101,

    KM_ERROR_UNKNOWN_ERROR = -1000,
} keymaster_error_t;

/**
 * keymaster_message - Serial header for communicating with KM server
 *
 * @cmd: the command, one of keymaster_command.
 * @payload: start of the serialized command specific payload
 */
struct keymaster_message {
    uint32_t cmd;
    uint8_t payload[0];
};

/**
 * km_no_response -  Generic keymaster response for commands with no special
 * response data
 *
 * @error: error code from command
 */
struct km_no_response {
    int32_t error;
};

/**
 * km_get_version_resp - response format for KM_GET_VERSION.
 */
struct km_get_version_resp {
    int32_t error;
    uint8_t major_ver;
    uint8_t minor_ver;
    uint8_t subminor_ver;
} TRUSTY_ATTR_PACKED;

/**
 * km_raw_buffer_resp - response format for a raw buffer
 */
struct km_raw_buffer_resp {
    int32_t error;
    uint32_t data_size;
    int8_t data[0];
} TRUSTY_ATTR_PACKED;

/**
 * km_get_mppubk_resp - response format for mppubk buffer
 */
struct km_get_mppubk_resp {
    int32_t error;
    uint32_t data_size;
    uint8_t data[64];
} TRUSTY_ATTR_PACKED;

/**
 * km_secure_unlock_data - represents the secure unlock data
 *
 * @serial_size: size of |serial_data|
 * @serial_data: serial_data (serial number)
 * @credential_size: size of |credential_data|
 * @credential_data: credential data
 */
struct km_secure_unlock_data {
    uint32_t serial_size;
    const uint8_t *serial_data;
    uint32_t credential_size;
    const uint8_t *credential_data;
} TRUSTY_ATTR_PACKED;
/**
 * km_set_ca_response_begin_req - starts the process to set the ATAP CA Response
 *
 * @ca_response_size: total size of the CA Response message
 */
struct km_set_ca_response_begin_req {
    uint32_t ca_response_size;
} TRUSTY_ATTR_PACKED;

/**
 * km_boot_params - Parameters sent from the bootloader to the Keymaster TA
 *
 * Since verified_boot_key_hash and verified_boot_hash have variable sizes, this
 * structure must be serialized before sending to the secure side
 * using km_boot_params_serialize().
 *
 * @os_version: OS version from Android image header
 * @os_patchlevel: OS patch level from Android image header
 * @device_locked: nonzero if device is locked
 * @verified_boot_state: one of keymaster_verified_boot_t
 * @verified_boot_key_hash_size: size of verified_boot_key_hash
 * @verified_boot_key_hash: hash of key used to verify Android image
 * @verified_boot_hash_size: size of verified_boot_hash
 * @verified_boot_hash: cumulative hash of all images verified thus far
 */
struct km_boot_params {
    uint32_t os_version;
    uint32_t os_patchlevel;
    uint32_t device_locked;
    uint32_t verified_boot_state;
    uint32_t verified_boot_key_hash_size;
    const uint8_t *verified_boot_key_hash;
    uint32_t verified_boot_hash_size;
    const uint8_t *verified_boot_hash;
} TRUSTY_ATTR_PACKED;

/**
 * km_attestation_data - represents a DER encoded key or certificate
 *
 * @algorithm: one of KM_ALGORITHM_RSA or KM_ALGORITHM_EC
 * @data_size: size of |data|
 * @data: DER encoded key or certificate (depending on operation)
 */
struct km_attestation_data {
    uint32_t algorithm;
    uint32_t data_size;
    const uint8_t *data;
} TRUSTY_ATTR_PACKED;

/**
 * km_raw_buffer - represents a single raw buffer
 *
 * @data_size: size of |data|
 * @data: pointer to the buffer
 */
struct km_raw_buffer {
    uint32_t data_size;
    const uint8_t *data;
} TRUSTY_ATTR_PACKED;

#endif /* TRUSTY_INTERFACE_KEYMASTER_H_ */

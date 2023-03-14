/*
 * Copyright 2023 NXP
 */

#ifndef TRUSTY_INTERFACE_MATTER_H_
#define TRUSTY_INTERFACE_MATTER_H_

#include <trusty/sysdeps.h>

#define MATTER_PORT "com.android.trusty.matter"
#define MATTER_MAX_BUFFER_LENGTH 4096

enum matter_command {
    MATTER_RESP_BIT              = 1,
    MATTER_STOP_BIT              = 2,
    MATTER_REQ_SHIFT             = 2,

    // bootloader commands
    MATTER_IMPORT_DAC            = (0x1 << MATTER_REQ_SHIFT),
    MATTER_IMPORT_PAI            = (0x2 << MATTER_REQ_SHIFT),
    MATTER_IMPORT_CD             = (0x3 << MATTER_REQ_SHIFT),
    MATTER_IMPORT_DAC_PRIKEY     = (0x4 << MATTER_REQ_SHIFT),
};

typedef enum {
    MATTER_ERROR_OK = 0,
    MATTER_ERROR_UNKNOWN_ERROR = -1,
} matter_error_t;

/**
 * matter_message - Serial header for communicating with Matter server
 *
 * @cmd: the command, one of matter_command.
 * @payload: start of the serialized command specific payload
 */
struct matter_message {
    uint32_t cmd;
    uint8_t payload[0];
};

/**
 * matter_no_response -  Generic matter response for commands with no special
 * response data
 *
 * @error: error code from command
 */
struct matter_no_response {
    int32_t error;
};

/**
 * matter_cert_data
 *
 * @data_size: size of |data|
 * @data: certificate data
 */
struct matter_cert_data {
    uint32_t data_size;
    const uint8_t *data;
} TRUSTY_ATTR_PACKED;

#endif /* TRUSTY_INTERFACE_MATTER_H_ */

# Queueless Trusty IPC

ql-tipc is a portable client library that implements Trusty queueless IPC.
It is intended to enable Trusty IPC in bootloader environments.

## Code organization

### IPC components

- libtipc - Functions to be called by library user
- ipc - IPC library
- ipc_dev - Helper functions for sending requests to the secure OS
- rpmb_proxy - Handles RPMB requests from secure storage service
- avb - Sends requests to the Android Verified Boot service

### Misc

- examples/ - Implementations of bootloader-specific code.
- arch/$ARCH/ - Architecture dependent implementation of Trusty device
   (see trusty_dev.h). Implements SMCs on ARM for example.

## Portability Notes

The suggested approach to porting ql-tipc is to copy all header and C files
into the bootloader and integrate as needed. RPMB storage operations and
functions defined in trusty/sysdeps.h require system dependent implementations.

If the TIPC_ENABLE_DEBUG preprocessor symbol is set, the code will include
debug information and run-time checks. Production builds should not use this.


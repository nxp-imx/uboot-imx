/*
 * Copyright (C) 2010-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef FSL_FASTBOOT_H
#define FSL_FASTBOOT_H

#define FASTBOOT_PTENTRY_FLAGS_REPEAT(n)              (n & 0x0f)
#define FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK            0x0000000F

/* Writes happen a block at a time.
   If the write fails, go to next block
   NEXT_GOOD_BLOCK and CONTIGOUS_BLOCK can not both be set */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_NEXT_GOOD_BLOCK  0x00000010

/* Find a contiguous block big enough for a the whole file
   NEXT_GOOD_BLOCK and CONTIGOUS_BLOCK can not both be set */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK 0x00000020

/* Write the file with write.i */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_I                0x00000100

/* Write the file with write.trimffs */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_TRIMFFS          0x00000200

/* Write the file as a series of variable/value pairs
   using the setenv and saveenv commands */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_ENV              0x00000400

#define FASTBOOT_MMC_BOOT_PARTITION_ID  1
#define FASTBOOT_MMC_USER_PARTITION_ID  0
#define FASTBOOT_MMC_NONE_PARTITION_ID -1

#define FASTBOOT_PARTITION_BOOT "boot"
#define FASTBOOT_PARTITION_RECOVERY "recovery"
#define FASTBOOT_PARTITION_SYSTEM "system"
#define FASTBOOT_PARTITION_BOOTLOADER "bootloader"
#define FASTBOOT_PARTITION_DATA "data"

enum {
    DEV_SATA,
    DEV_MMC,
    DEV_NAND
};

struct cmd_fastboot_interface {
	/* This function is called when a buffer has been
	   recieved from the client app.
	   The buffer is a supplied by the board layer and must be unmodified.
	   The buffer_size is how much data is passed in.
	   Returns 0 on success
	   Returns 1 on failure

	   Set by cmd_fastboot	*/
	int (*rx_handler)(const unsigned char *buffer,
			  unsigned int buffer_size);

	/* This function is called when an exception has
	   occurred in the device code and the state
	   off fastboot needs to be reset

	   Set by cmd_fastboot */
	void (*reset_handler)(void);

	/* A getvar string for the product name
	   It can have a maximum of 60 characters

	   Set by board	*/
	char *product_name;

	/* A getvar string for the serial number
	   It can have a maximum of 60 characters

	   Set by board */
	char *serial_no;

	/* Nand block size
	   Supports the write option WRITE_NEXT_GOOD_BLOCK

	   Set by board */
	unsigned int nand_block_size;

	/* Nand oob size
	   Set by board */
	unsigned int nand_oob_size;

	/* Transfer buffer, for handling flash updates
	   Should be multiple of the nand_block_size
	   Care should be take so it does not overrun bootloader memory
	   Controlled by the configure variable CFG_FASTBOOT_TRANSFER_BUFFER

	   Set by board */
	unsigned char *transfer_buffer;

	/* How big is the transfer buffer
	   Controlled by the configure variable
	   CFG_FASTBOOT_TRANSFER_BUFFER_SIZE

	   Set by board	*/
	unsigned int transfer_buffer_size;

};

/* flash partitions are defined in terms of blocks
** (flash erase units)
*/
struct fastboot_ptentry {
	/* The logical name for this partition, null terminated */
	char name[16];
	/* The start wrt the nand part, must be multiple of nand block size */
	unsigned int start;
	/* The length of the partition, must be multiple of nand block size */
	unsigned int length;
	/* Controls the details of how operations are done on the partition
	   See the FASTBOOT_PTENTRY_FLAGS_*'s defined below */
	unsigned int flags;
	/* partition id: 0 - normal partition; 1 - boot partition */
	unsigned int partition_id;
	/* partition number in block device */
	unsigned int partition_index;
};

struct fastboot_device_info {
	unsigned char type;
	unsigned char dev_id;
};

extern struct fastboot_device_info fastboot_devinfo;

/* Prepare the fastboot environments,
  * should be executed before "fastboot" cmd
  */
void fastboot_setup(void);


/* The Android-style flash handling */

/* tools to populate and query the partition table */
void fastboot_flash_add_ptn(struct fastboot_ptentry *ptn);
struct fastboot_ptentry *fastboot_flash_find_ptn(const char *name);
struct fastboot_ptentry *fastboot_flash_get_ptn(unsigned n);
unsigned int fastboot_flash_get_ptn_count(void);
void fastboot_flash_dump_ptn(void);


/* Check the board special boot mode reboot to fastboot mode. */
int fastboot_check_and_clean_flag(void);

/* Set the flag which reboot to fastboot mode*/
void fastboot_enable_flag(void);

/*check if fastboot mode is requested by user*/
void check_fastboot(void);

/*Setup board-relative fastboot environment */
void board_fastboot_setup(void);

#ifdef CONFIG_FASTBOOT_STORAGE_NAND
/*Save parameters for NAND storage partitions */
void save_parts_values(struct fastboot_ptentry *ptn,
	unsigned int offset, unsigned int size);

/* Checks parameters for NAND storage partitions
  * Return 1 if the parameter is not set
  * Return 0 if the parameter has been set
  */
int check_parts_values(struct fastboot_ptentry *ptn);
#endif /*CONFIG_FASTBOOT_STORAGE_NAND*/

#endif /* FSL_FASTBOOT_H */

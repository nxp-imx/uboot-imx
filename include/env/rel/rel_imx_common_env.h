#ifndef __REL_IMX91_COMMON_H
#define __REL_IMX91_COMMON_H

/* Pool of randomly generated UUIDs at host machine */
#define RANDOM_UUIDS	\
    "uuid_disk=f8a2b3c4-5d6e-7f80-9102-3456789abcde\0" \
    "part1_uuid=a1b2c3d4-e5f6-4789-a012-3456789abcde\0" \
    "part2_uuid=b2c3d4e5-f6a7-4890-b123-456789abcdef\0" \
    "part3_uuid=c3d4e5f6-a7b8-4901-c234-56789abcdef0\0" \
    "part4_uuid=d4e5f6a7-b8c9-4012-d345-6789abcdef01\0" \
    "part5_uuid=e5f6a7b8-c9da-4123-e456-789abcdef012\0" \
    "part6_uuid=f6a7b8c9-daeb-4234-f567-89abcdef0123\0" \
    "part7_uuid=a7b8c9da-ebfc-4345-a678-9abcdef01234\0" \
    "part8_uuid=b8c9daeb-fcad-4456-b789-abcdef012345\0" \
    "part9_uuid=c9daebbf-adbe-4567-c89a-bcdef0123456\0" \
    "part10_uuid=daebfcad-becf-4678-d9ab-cdef01234567\0"

#define LINUX_4GB_PARTITION_TABLE \
	"\"uuid_disk=${uuid_disk};" \
	"start=8MiB," \
	"name=linux,size=64MiB,uuid=${part1_uuid};" \
	"name=recovery,size=64MiB,uuid=${part2_uuid};" \
	"name=rootfs,size=800MiB,uuid=${part3_uuid};" \
	"name=update,size=800MiB,uuid=${part4_uuid};" \
	"name=data,size=-,uuid=${part5_uuid};" \
	"\""

#define LINUX_DUALBOOT_4GB_PARTITION_TABLE \
	"\"uuid_disk=${uuid_disk};" \
	"start=8MiB," \
	"name=linux_a,size=64MiB,uuid=${part1_uuid};" \
	"name=linux_b,size=64MiB,uuid=${part2_uuid};" \
	"name=rootfs_a,size=800MiB,type=linux,uuid=${part3_uuid};" \
	"name=rootfs_b,size=800MiB,type=linux,uuid=${part4_uuid};" \
	"name=data,size=-,type=linux,uuid=${part5_uuid};" \
	"\""

#define CFG_EXTRA_ENV_SETTINGS	\
	"parts_linux_dualboot=" LINUX_DUALBOOT_4GB_PARTITION_TABLE "\0" \
	"parts_linux=" LINUX_4GB_PARTITION_TABLE "\0" \
	RANDOM_UUIDS

#endif
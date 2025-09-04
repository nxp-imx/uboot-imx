// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2020, 2023, 2025 NXP
 * Copyright 2024 Mathieu Othacehe <othacehe@gnu.org>
 *
 */

#include <asm/io.h>
#include <asm/mach-imx/sys_proto.h>
#include <asm/mach-imx/ele_api.h>
#include <dm.h>
#include <malloc.h>
#include <memalign.h>
#include <misc.h>
#include <memalign.h>
#include <linux/delay.h>
#include <time.h>

DECLARE_GLOBAL_DATA_PTR;

static u32 compute_crc(const struct ele_msg *msg)
{
	u32 crc = 0;
	size_t i = 0;
	u32 *data = (u32 *)msg;

	for (i = 0; i < (msg->size - 1); i++)
		crc ^= data[i];

	return crc;
}

int ele_release_rdc(u8 core_id, u8 xrdc, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_RELEASE_RDC_REQ;
	switch (xrdc) {
	case 0:
		msg.data[0] = (0x74 << 8) | core_id;
		break;
	case 1:
		msg.data[0] = (0x78 << 8) | core_id;
		break;
	case 2:
		msg.data[0] = (0x82 << 8) | core_id;
		break;
	case 3:
		msg.data[0] = (0x86 << 8) | core_id;
		break;
	default:
		printf("Error: wrong xrdc index %u\n", xrdc);
		return -EINVAL;
	}

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, core id %u, response 0x%x\n",
		       __func__, ret, core_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_auth_oem_ctnr(ulong ctnr_addr, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 3;
	msg.command = ELE_OEM_CNTN_AUTH_REQ;
	msg.data[0] = upper_32_bits(ctnr_addr);
	msg.data[1] = lower_32_bits(ctnr_addr);

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, cntr_addr 0x%lx, response 0x%x\n",
		       __func__, ret, ctnr_addr, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_release_container(u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_RELEASE_CONTAINER_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_verify_image(u32 img_id, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_VERIFY_IMAGE_REQ;
	msg.data[0] = 1 << img_id;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, img_id %u, response 0x%x\n",
		       __func__, ret, img_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_forward_lifecycle(u16 life_cycle, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_FWD_LIFECYCLE_UP_REQ;
	msg.data[0] = life_cycle;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, life_cycle 0x%x, response 0x%x\n",
		       __func__, ret, life_cycle, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_read_common_fuse(u16 fuse_id, u32 *fuse_words, u32 fuse_num, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!fuse_words) {
		printf("Invalid parameters for fuse read\n");
		return -EINVAL;
	}

	if (is_imx8ulp() && ((fuse_id != 1 && fuse_num != 1) || (fuse_id == 1 && fuse_num != 4))) {
		printf("Invalid fuse number parameter\n");
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_READ_FUSE_REQ;
	msg.data[0] = fuse_id;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, fuse_id 0x%x, response 0x%x\n",
		       __func__, ret, fuse_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	fuse_words[0] = msg.data[1];
	if (fuse_id == 1 && is_imx8ulp()) {
		/* OTP_UNIQ_ID */
		fuse_words[1] = msg.data[2];
		fuse_words[2] = msg.data[3];
		fuse_words[3] = msg.data[4];
	}

	return ret;
}

int ele_write_fuse(u16 fuse_id, u32 fuse_val, bool lock, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 3;
	msg.command = ELE_WRITE_FUSE_REQ;
	msg.data[0] = (32 << 16) | (fuse_id << 5);
	if (lock)
		msg.data[0] |= (1 << 31);

	msg.data[1] = fuse_val;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, fuse_id 0x%x, response 0x%x\n",
		       __func__, ret, fuse_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_write_shadow_fuse(u32 fuse_id, u32 fuse_val, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 3;
	msg.command = ELE_WRITE_SHADOW_REQ;
	msg.data[0] = fuse_id;
	msg.data[1] = fuse_val;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, fuse_id 0x%x, response 0x%x\n",
		       __func__, ret, fuse_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_read_shadow_fuse(u32 fuse_id, u32 *fuse_val, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!fuse_val) {
		printf("Invalid parameters for shadow read\n");
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_READ_SHADOW_REQ;
	msg.data[0] = fuse_id;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, fuse_id 0x%x, response 0x%x\n",
		       __func__, ret, fuse_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	*fuse_val = msg.data[1];

	return ret;
}

int ele_release_caam(u32 core_did, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_RELEASE_CAAM_REQ;
	msg.data[0] = core_did;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_get_fw_version(u32 *fw_version, u32 *sha1, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!fw_version) {
		printf("Invalid parameters for f/w version read\n");
		return -EINVAL;
	}

	if (!sha1) {
		printf("Invalid parameters for commit sha1\n");
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_GET_FW_VERSION_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	*fw_version = msg.data[1];
	*sha1 = msg.data[2];

	return ret;
}

int ele_dump_buffer(u32 *buffer, u32 buffer_length)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret, i = 0;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_DUMP_DEBUG_BUFFER_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret) {
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

		return ret;
	}

	if (buffer) {
		buffer[i++] = *(u32 *)&msg; /* Need dump the response header */
		for (; i < buffer_length && i < msg.size; i++)
			buffer[i] = msg.data[i - 1];
	}

	return i;
}

int ele_get_info(struct ele_get_info_data *info, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 4;
	msg.command = ELE_GET_INFO_REQ;
	msg.data[0] = upper_32_bits((ulong)info);
	msg.data[1] = lower_32_bits((ulong)info);
	msg.data[2] = sizeof(struct ele_get_info_data);

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_get_fw_status(u32 *status, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_GET_FW_STATUS_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	*status = msg.data[1] & 0xF;

	return ret;
}

int ele_release_m33_trout(void)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_ENABLE_RTC_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	return ret;
}

int ele_enable_aux(enum ELE_AUX_ID id)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_ENABLE_AUX_REQ;
	msg.data[0] = id;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	return ret;
}


int ele_get_events(u32 *events, u32 *events_cnt, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret, i = 0;
	u32 actual_events;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!events || !events_cnt || *events_cnt == 0) {
		printf("Invalid parameters for %s\n", __func__);
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_GET_EVENTS_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	if (!ret) {
		actual_events = msg.data[1] & 0xffff;
		if (*events_cnt < actual_events)
			actual_events = *events_cnt;

		for (; i < actual_events; i++)
			events[i] = msg.data[i + 2];

		*events_cnt = actual_events;
	}

	return ret;
}

int ele_start_rng(void)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_START_RNG;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	return ret;
}

int ele_derive_huk(u8 *key, size_t key_size, u8 *seed, size_t seed_size)
{
	struct udevice *dev = gd->arch.ele_dev;
	struct ele_msg msg;
	int msg_size = sizeof(struct ele_msg);
	u8 *seed_aligned, *key_aligned;
	int ret, size;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (key_size != 16 && key_size != 32) {
		printf("key size can only be 16 or 32\n");
		return -EINVAL;
	}

	if (seed_size >= (1U << 16) - 1) {
		printf("seed size is too large\n");
		return -EINVAL;
	}

	seed_aligned = memalign(ARCH_DMA_MINALIGN, seed_size);
	if (!seed_aligned) {
		printf("failed to alloc memory\n");
		return -EINVAL;
	}
	memcpy(seed_aligned, seed, seed_size);

	key_aligned = memalign(ARCH_DMA_MINALIGN, key_size);
	if (!key_aligned) {
		printf("failed to alloc memory\n");
		ret = -EINVAL;
		goto ret_seed;
	}

	size = ALIGN(seed_size, ARCH_DMA_MINALIGN);
	flush_dcache_range((ulong)seed_aligned,
			   (ulong)seed_aligned + size);

	size = ALIGN(key_size, ARCH_DMA_MINALIGN);
	invalidate_dcache_range((ulong)key_aligned,
				(ulong)key_aligned + size);

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 7;
	msg.command = ELE_CMD_DERIVE_KEY;
	msg.data[0] = upper_32_bits((ulong)key_aligned);
	msg.data[1] = lower_32_bits((ulong)key_aligned);
	msg.data[2] = upper_32_bits((ulong)seed_aligned);
	msg.data[3] = lower_32_bits((ulong)seed_aligned);
	msg.data[4] = seed_size << 16 | key_size;
	msg.data[5] = compute_crc(&msg);

	ret = misc_call(dev, false, &msg, msg_size, &msg, msg_size);
	if (ret) {
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);
		goto ret_key;
	}

	invalidate_dcache_range((ulong)key_aligned,
				(ulong)key_aligned + size);
	memcpy(key, key_aligned, key_size);

ret_key:
	free(key_aligned);
ret_seed:
	free(seed_aligned);

	return ret;
}

int ele_commit(u16 fuse_id, u32 *response, u32 *info_type)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret = 0;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_COMMIT_REQ;
	msg.data[0] = fuse_id;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, fuse_id 0x%x, response 0x%x\n",
		       __func__, ret, fuse_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	if (info_type)
		*info_type = msg.data[1];

	return ret;
}

int ele_write_secure_fuse(ulong signed_msg_blk, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 3;
	msg.command = ELE_WRITE_SECURE_FUSE_REQ;

	msg.data[0] = upper_32_bits(signed_msg_blk);
	msg.data[1] = lower_32_bits(signed_msg_blk);

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x, failed fuse row index %u\n",
		       __func__, ret, msg.data[0], msg.data[1]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_return_lifecycle_update(ulong signed_msg_blk, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 3;
	msg.command = ELE_RET_LIFECYCLE_UP_REQ;

	msg.data[0] = upper_32_bits(signed_msg_blk);
	msg.data[1] = lower_32_bits(signed_msg_blk);

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_generate_dek_blob(u32 key_id, u32 src_paddr, u32 dst_paddr, u32 max_output_size)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 8;
	msg.command = ELE_GENERATE_DEK_BLOB;
	msg.data[0] = key_id;
	msg.data[1] = 0x0;
	msg.data[2] = src_paddr;
	msg.data[3] = 0x0;
	msg.data[4] = dst_paddr;
	msg.data[5] = max_output_size;
	msg.data[6] = compute_crc(&msg);

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret 0x%x, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	return ret;
}

int ele_blob(u32 key_id, u32 src, u32 in_size, u32 dst, u32 out_size, int wrap_blob)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {0};
	int ret = 0;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = ELE_BLOB_MSG_SIZE;
	msg.command = ELE_BLOB;
	msg.data[0] = upper_32_bits(src);
	msg.data[1] = lower_32_bits(src);
	msg.data[2] = upper_32_bits(dst);
	msg.data[3] = lower_32_bits(dst);
	msg.data[4] = 0;
	msg.data[5] = 0;
	msg.data[6] = in_size;
	msg.data[7] = out_size;
	msg.data[8] = 0;
	if (wrap_blob)
		msg.data[9] = (ELE_BLOB_ENCAP_MODE << ELE_BLOB_SHIFT);
	else
		msg.data[9] = (ELE_BLOB_DECAP_MODE << ELE_BLOB_SHIFT);
	msg.data[10] = compute_crc(&msg);

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret 0x%x, response 0x%x\n",
		       __func__, ret, msg.data[0]);
	return ret;
}

int ele_v2x_get_state(struct v2x_get_state *state, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_V2X_GET_STATE_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	state->v2x_state = msg.data[1] & 0xFF;
	state->v2x_power_state = (msg.data[1] & 0xFF00) >> 8;
	state->v2x_err_code = msg.data[2];

	return ret;
}

int ele_volt_change_start_req(void)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_VOLT_CHANGE_START_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	return ret;
}

int ele_volt_change_finish_req(void)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_VOLT_CHANGE_FINISH_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	return ret;
}

int ele_message_call(struct ele_msg *msg)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	int ret = -EINVAL;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	/* Call pre-prepared ELE message. */
	ret = misc_call(dev, false, msg, size, msg, size);
	if (ret)
		printf("Error: %s: ret 0x%x, response 0x%x\n",
		       __func__, ret, msg->data[0]);

	return ret;
}

int ele_set_gmid(u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_SET_GMID_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_get_hw_unique_key(uint8_t *hwkey, size_t key_size, uint8_t *ctx, size_t ctx_size)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg;
	uint8_t *ctx_addr = NULL;
	uint8_t *key_addr = NULL;
	int ret = -EINVAL;

	if (!dev) {
		printf("s400 dev is not initialized\n");
		return -ENODEV;
	}

	/* sanity check the key and context */
	if (ctx_size >= (1U << 16) - 1) {
		printf("%s: Invalid context size!\n", __func__);
		return -EINVAL;
	}
	if ((key_size != 16) && (key_size != 32)) {
		printf("%s: Invalid key size!\n", __func__);
		return -EINVAL;
	}
	if (!hwkey || !ctx) {
		printf("%s: invalid input buffer!\n", __func__);
		return -EINVAL;
	}

	/* alloc temp buffer for input context in case it's not cacheline aligned */
	ctx_addr = memalign(ARCH_DMA_MINALIGN, ctx_size);
	if (!ctx_addr) {
		printf("%s: Fail to alloc memory!\n", __func__);
		return -EINVAL;
	}
	memcpy(ctx_addr, ctx, ctx_size);

	/* key buffer */
	key_addr = memalign(ARCH_DMA_MINALIGN, key_size);
	if (!key_addr) {
		printf("%s: Fail to alloc memory!\n", __func__);
		goto exit;
	}

	flush_dcache_range((unsigned long)ctx_addr, ALIGN((unsigned long)ctx_addr + ctx_size, ARCH_DMA_MINALIGN));
	invalidate_dcache_range((unsigned long)key_addr, ALIGN((unsigned long)key_addr + key_size, ARCH_DMA_MINALIGN));

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 7;
	msg.command = ELE_CMD_DERIVE_KEY;
	msg.data[0] = upper_32_bits((ulong)key_addr);
	msg.data[1] = lower_32_bits((ulong)key_addr);
	msg.data[2] = upper_32_bits((ulong)ctx_addr);
	msg.data[3] = lower_32_bits((ulong)ctx_addr);
	msg.data[4] = ((ctx_size << 16) | key_size);
	msg.data[5] = compute_crc(&msg);

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret) {
		printf("Error: %s: ret 0x%x, response 0x%x\n",
		       __func__, ret, msg.data[0]);
		goto exit;
	}

	invalidate_dcache_range((unsigned long)key_addr, ALIGN((unsigned long)key_addr + key_size, ARCH_DMA_MINALIGN));
	memcpy(hwkey, key_addr, key_size);
	ret = 0;

exit:
	if (ctx_addr)
		free(ctx_addr);
	if (key_addr)
		free(key_addr);

	return ret;
}

#define IMX_ELE_TRNG_STATUS_READY 0x3
#define IMX_ELE_CSAL_STATUS_READY 0x2
int ele_get_trng_state(void)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	struct ele_trng_state {
		uint32_t rsp_code;
		uint8_t trng_state;
		uint8_t csal_state;
		uint16_t rsv;
	} *rsp = NULL;

	if (!dev) {
		printf("s400 dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_GET_TRNG_STATE;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret) {
		printf("Error: %s: ret 0x%x, response 0x%x\n",
		       __func__, ret, msg.data[0]);
		return ret;
	}

	rsp = (void *)msg.data;
	if (rsp->trng_state != IMX_ELE_TRNG_STATUS_READY ||
		rsp->csal_state != IMX_ELE_CSAL_STATUS_READY) {
		return -EBUSY;
	} else {
		return 0;
	}
}

int ele_get_random(u32 src_paddr, size_t len)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;
	u32 start = 0;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (src_paddr == 0 || len == 0) {
		printf("Wrong input parameter!\n");
		return -EINVAL;
	}

	/* make sure the ELE is ready to produce RNG */
	start = get_timer(0);
	while ((ele_get_trng_state() != 0)) {
		/* timeout in 5ms */
		if (get_timer(start) >= 5) {
			printf("get random timeout!\n");
			return -EBUSY;
		}
		udelay(100);
	}

	flush_dcache_range((ulong)src_paddr,
				ALIGN((ulong)src_paddr + len, ARCH_DMA_MINALIGN));

	msg.version = ELE_VERSION_FW;
	msg.tag = ELE_CMD_TAG;
	msg.size = 4;
	msg.command = ELE_GET_RNG;
	msg.data[0] = 0;
	msg.data[1] = src_paddr;
	msg.data[2] = len;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret 0x%x, response 0x%x\n",
		       __func__, ret, msg.data[0]);
	else
		invalidate_dcache_range((ulong)src_paddr,
				ALIGN((ulong)src_paddr + len, ARCH_DMA_MINALIGN));

	return ret;
}

#if IS_ENABLED(CONFIG_IMX_CRRM)
int ele_crrm_init(u8 *action, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!action) {
		printf("Invalid parameters for CRRM init\n");
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_CRRM_INIT_REQ;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret) {
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);
		goto end;
	}

	*action = msg.data[1] & 0xFF;

end:
	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_crrm_get_boot_mode(enum CRRM_BOOT_MODE *boot_mode, u8 *timer_id, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!boot_mode || !timer_id) {
		printf("Invalid parameters for CRRM get boot mode\n");
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_CRRM_GET_BOOTMODE;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret) {
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);
		goto end;
	}

	*boot_mode = msg.data[1] & 0xFF;
	*timer_id = (msg.data[1] >> 8) & 0xFF;

end:
	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_crrm_protect_image(u8 media_id, u32 start_addr, u32 length, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 4;
	msg.command = ELE_CRRM_PROTECT_BOOTIMG;
	msg.data[0] = media_id;
	msg.data[1] = start_addr;
	msg.data[2] = length;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_crrm_set_luts(u8 media_id, u8 luts_num, u32 luts_addr, u32 luts_size, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 4;
	msg.command = ELE_CRRM_SET_LUTS;
	msg.data[0] = (media_id << 8) | luts_num;
	msg.data[1] = luts_addr;
	msg.data[2] = luts_size;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_crrm_change_lut(u8 media_id, u8 lut_index, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_CRRM_CHANGE_LUT;
	msg.data[0] = (media_id << 8) | lut_index;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_crrm_init_awdt(u8 timer_id, u8 operation, u32 config_addr, u32 config_size, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 4;
	msg.command = ELE_CRRM_INIT_AWDT;
	msg.data[0] = (operation << 8) | timer_id;
	msg.data[1] = config_addr;
	msg.data[2] = config_size;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_crrm_set_boot_mode(enum CRRM_BOOT_MODE *boot_mode, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!boot_mode) {
		printf("Invalid parameters for CRRM set boot mode\n");
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_CRRM_SET_BOOTMODE;
	msg.data[0] = *boot_mode;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret) {
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);
		goto end;
	}

	*boot_mode = msg.data[1] & 0xFF;

end:
	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_crrm_refresh_awdt(u8 timer_id, u8 pub_key, u32 data_addr, u32 data_size, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 4;
	msg.command = ELE_CRRM_REFRESH_AWDT;
	msg.data[0] = (pub_key << 8) | timer_id;
	msg.data[1] = data_addr;
	msg.data[2] = data_size;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret)
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

int ele_crrm_get_nonce(u8 timer_id, u32 nonce_buf, u32 *buf_size, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!nonce_buf || !buf_size || !*buf_size) {
		printf("Wrong parameters for get nonce\n");
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 4;
	msg.command = ELE_CRRM_GET_NONCE;
	msg.data[0] = timer_id;
	msg.data[1] = nonce_buf;
	msg.data[2] = *buf_size;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret) {
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);
		goto end;
	}

	*buf_size = msg.data[1];

end:
	if (response)
		*response = msg.data[0];

	return ret;
}


int ele_crrm_get_status(u8 timer_id, u32 status_addr, u32 *status_size, u32 *response)
{
	struct udevice *dev = gd->arch.ele_dev;
	int size = sizeof(struct ele_msg);
	struct ele_msg msg = {};
	int ret;

	if (!dev) {
		printf("ele dev is not initialized\n");
		return -ENODEV;
	}

	if (!status_size) {
		printf("Invalid parameters for CRRM get status\n");
		return -EINVAL;
	}

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 4;
	msg.command = ELE_CRRM_GET_STATUS;
	msg.data[0] = timer_id;
	msg.data[1] = status_addr;
	msg.data[2] = *status_size;

	ret = misc_call(dev, false, &msg, size, &msg, size);
	if (ret) {
		printf("Error: %s: ret %d, response 0x%x\n",
		       __func__, ret, msg.data[0]);
		goto end;
	}

	*status_size = msg.data[1];

end:
	if (response)
		*response = msg.data[0];

	return ret;
}
#endif

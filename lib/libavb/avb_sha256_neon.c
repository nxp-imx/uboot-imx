/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2024 NXP.
 */


#include <linux/types.h>
#include <arm_neon.h>
#include "avb_sha.h"

static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void SHA256_transform_NEON(AvbSHA256Ctx* ctx,
                             const uint8_t* message,
                             size_t block_nb)
{
	uint32x4_t STATE0, STATE1, ABEF_SAVE, CDGH_SAVE;
	uint32x4_t MSG0, MSG1, MSG2, MSG3;
	uint32x4_t TMP0, TMP1, TMP2;
	uint32_t* state = ctx->h;
	uint32_t length = block_nb * 64;
	const uint8_t*  data = message;
	const uint32_t* K = sha256_k;

	/* Load state */
	STATE0 = vld1q_u32(&state[0]);
	STATE1 = vld1q_u32(&state[4]);

	while (length >= 64)
	{
		/* Save state */
		ABEF_SAVE = STATE0;
		CDGH_SAVE = STATE1;

		/* Load message */
		MSG0 = vld1q_u32((const uint32_t *)(data +  0));
		MSG1 = vld1q_u32((const uint32_t *)(data + 16));
		MSG2 = vld1q_u32((const uint32_t *)(data + 32));
		MSG3 = vld1q_u32((const uint32_t *)(data + 48));

		/* Reverse for little endian */
		MSG0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG0)));
		MSG1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG1)));
		MSG2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG2)));
		MSG3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG3)));

		TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0x00]));

		/* Rounds 0-3 */
		MSG0 = vsha256su0q_u32(MSG0, MSG1);
		TMP2 = STATE0;
		TMP1 = vaddq_u32(MSG1, vld1q_u32(&K[0x04]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
		MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

		/* Rounds 4-7 */
		MSG1 = vsha256su0q_u32(MSG1, MSG2);
		TMP2 = STATE0;
		TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[0x08]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
		MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

		/* Rounds 8-11 */
		MSG2 = vsha256su0q_u32(MSG2, MSG3);
		TMP2 = STATE0;
		TMP1 = vaddq_u32(MSG3, vld1q_u32(&K[0x0c]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
		MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

		/* Rounds 12-15 */
		MSG3 = vsha256su0q_u32(MSG3, MSG0);
		TMP2 = STATE0;
		TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0x10]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
		MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

		/* Rounds 16-19 */
		MSG0 = vsha256su0q_u32(MSG0, MSG1);
		TMP2 = STATE0;
		TMP1 = vaddq_u32(MSG1, vld1q_u32(&K[0x14]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
		MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

		/* Rounds 20-23 */
		MSG1 = vsha256su0q_u32(MSG1, MSG2);
		TMP2 = STATE0;
		TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[0x18]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
		MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

		/* Rounds 24-27 */
		MSG2 = vsha256su0q_u32(MSG2, MSG3);
		TMP2 = STATE0;
		TMP1 = vaddq_u32(MSG3, vld1q_u32(&K[0x1c]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
		MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

		/* Rounds 28-31 */
		MSG3 = vsha256su0q_u32(MSG3, MSG0);
		TMP2 = STATE0;
		TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0x20]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
		MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

		/* Rounds 32-35 */
		MSG0 = vsha256su0q_u32(MSG0, MSG1);
		TMP2 = STATE0;
		TMP1 = vaddq_u32(MSG1, vld1q_u32(&K[0x24]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
		MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

		/* Rounds 36-39 */
		MSG1 = vsha256su0q_u32(MSG1, MSG2);
		TMP2 = STATE0;
		TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[0x28]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
		MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

		/* Rounds 40-43 */
		MSG2 = vsha256su0q_u32(MSG2, MSG3);
		TMP2 = STATE0;
		TMP1 = vaddq_u32(MSG3, vld1q_u32(&K[0x2c]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
		MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

		/* Rounds 44-47 */
		MSG3 = vsha256su0q_u32(MSG3, MSG0);
		TMP2 = STATE0;
		TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0x30]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
		MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

		/* Rounds 48-51 */
		TMP2 = STATE0;
		TMP1 = vaddq_u32(MSG1, vld1q_u32(&K[0x34]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

		/* Rounds 52-55 */
		TMP2 = STATE0;
		TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[0x38]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);

		/* Rounds 56-59 */
		TMP2 = STATE0;
		TMP1 = vaddq_u32(MSG3, vld1q_u32(&K[0x3c]));
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

		/* Rounds 60-63 */
		TMP2 = STATE0;
		STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
		STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);

		/* Combine state */
		STATE0 = vaddq_u32(STATE0, ABEF_SAVE);
		STATE1 = vaddq_u32(STATE1, CDGH_SAVE);

		data += 64;
		length -= 64;
	}

	/* Save state */
	vst1q_u32(&state[0], STATE0);
	vst1q_u32(&state[4], STATE1);
}

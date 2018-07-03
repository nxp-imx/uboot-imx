/*
 * Copyright (c) 2012-2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 * Copyright 2018 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CAAM_H__
#define	__CAAM_H__

//! @name Error codes
//@{
#if !defined(SUCCESS)
#define SUCCESS (0)
#endif

#define ERROR_ANY           (-1)
#define ERROR_IN_PAGE_ALLOC (1)


////////////////////////////////////////////////////////////////////////////////
//! @brief Enable and initialize the CAAM driver.
//!
//! This function enables the clock to the CAAM. It initializes the RNG, and
//! instantiate it to allow generation of key for blob.
//!
////////////////////////////////////////////////////////////////////////////////
void caam_open(void);
void imx_sec_init(void);

////////////////////////////////////////////////////////////////////////////////
//! @brief Generate a blob of a secure key.
//!
//! @param[in] plain_data_addr  Location address of the plain text data.
//! @param[in] blob_addr  Location address of the blob.
//! @param[in] size  	Size in bytes of the data to encrypt.
//!
//! @return SUCCESS
//! @return ERROR_XXX
////////////////////////////////////////////////////////////////////////////////
uint32_t caam_gen_blob(uint32_t plain_data_addr, uint32_t blob_addr, uint32_t size);

////////////////////////////////////////////////////////////////////////////////
//! @brief Decapsulate a blob of a secure key.
//!
//! @param[in] blob_addr  Location address of the blob.
//!
//! @return SUCCESS
//! @return ERROR_XXX
////////////////////////////////////////////////////////////////////////////////
uint32_t caam_decap_blob(uint32_t plain_text, uint32_t blob_addr, uint32_t size);
uint32_t caam_hwrng(uint8_t *output_ptr, uint32_t output_len);

#endif /* __CAAM_H__ */

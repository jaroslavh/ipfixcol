/**
 * \file tests/unit_tests/src/convertors.cpp
 * \author Lukas Hutak <xhutak01@stud.fit.vutbr.cz>
 * \brief Convertor tests
 */
/* Copyright (C) 2016-2017 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

/**
 * \defgroup ipx_convertors_test Data conversion tests
 *
 * \note In many cases, test functions use dynamically allocated variables,
 *   because it is usefull for valgrind memory check (accessing an array out
 *   of bounds, etc.)
 * @{
 */

#include <gtest/gtest.h>
#include <cstring>
#include <endian.h>

extern "C" {
	#include <ipfixcol2/convertors.h>
}

#define BYTES_1 (1U)
#define BYTES_2 (2U)
#define BYTES_3 (3U)
#define BYTES_4 (4U)
#define BYTES_5 (5U)
#define BYTES_6 (6U)
#define BYTES_7 (7U)
#define BYTES_8 (8U)

// Auxiliary definitions of maximal values for UINT_XX for 3, 5, 6 and 7 bytes
const uint32_t IPX_UINT24_MAX = 0xFFFFFFUL;
const uint64_t IPX_UINT40_MAX = 0x000000FFFFFFFFFFULL;
const uint64_t IPX_UINT48_MAX = 0x0000FFFFFFFFFFFFULL;
const uint64_t IPX_UINT56_MAX = 0x00FFFFFFFFFFFFFFULL;

const int32_t IPX_INT24_MAX = 0x007FFFFFL;
const int64_t IPX_INT40_MAX = 0x0000007FFFFFFFFFLL;
const int64_t IPX_INT48_MAX = 0x00007FFFFFFFFFFFLL;
const int64_t IPX_INT56_MAX = 0x007FFFFFFFFFFFFFLL;

const int32_t IPX_INT24_MIN = 0xFF800000L;
const int64_t IPX_INT40_MIN = 0xFFFFFF8000000000LL;
const int64_t IPX_INT48_MIN = 0xFFFF800000000000LL;
const int64_t IPX_INT56_MIN = 0xFF80000000000000LL;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/**
 * \brief Test fixture for SetUint tests
 */
class ConverterUint : public ::testing::Test {
protected:
	/*
	 * We want to have all variables dynamically allocated so Valgrind can check
	 * access out of bounds, etc.
     */
	uint8_t  *u8;
	uint16_t *u16;
	uint32_t *u32;
	uint64_t *u64;

	uint8_t *u24;
	uint8_t *u40;
	uint8_t *u48;
	uint8_t *u56;

public:
	/** Create variables for tests */
	virtual void SetUp() {
		u8  = new uint8_t;
		u16 = new uint16_t;
		u32 = new uint32_t;
		u64 = new uint64_t;

		u24 = new uint8_t[3];
		u40 = new uint8_t[5];
		u48 = new uint8_t[6];
		u56 = new uint8_t[7];
	}

	/** Destroy variables for the tests */
	virtual void TearDown() {
		delete u8;
		delete u16;
		delete u32;
		delete u64;

		delete[] u24;
		delete[] u40;
		delete[] u48;
		delete[] u56;
	}
};

/*
 * Insert the maximum possible value i.e. "UINT64_MAX" and the minimum possible
 * value i.e. "0" into 1 - 8 byte variables.
 */
TEST_F(ConverterUint, SetUintMaxMin) {
	// SetUp
	const uint64_t max_val = UINT64_MAX;
	const uint64_t min_val = 0U;

	// Execute
	// 1 byte
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1,  max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*u8,  UINT8_MAX);
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1,  min_val), 0);
	EXPECT_EQ(*u8,  0U);

	// 2 bytes
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*u16, UINT16_MAX);
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, min_val), 0);
	EXPECT_EQ(*u16, 0U);

	// 4 bytes
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*u32, UINT32_MAX);
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, min_val), 0);
	EXPECT_EQ(*u32, 0U);

	// 8 bytes
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, max_val), 0);
	EXPECT_EQ(*u64, UINT64_MAX);
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, min_val), 0);
	EXPECT_EQ(*u64, 0U);

	// Other (unusual situations i.e. 3, 5, 6 and 7 bytes)
	// 3 bytes
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(memcmp(u24, &max_val, BYTES_3), 0);
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, min_val), 0);
	EXPECT_EQ(memcmp(u24, &min_val, BYTES_3), 0);

	// 5 bytes
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(memcmp(u40, &max_val, BYTES_5), 0);
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, min_val), 0);
	EXPECT_EQ(memcmp(u40, &min_val, BYTES_5), 0);

	// 6 bytes
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(memcmp(u48, &max_val, BYTES_6), 0);
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, min_val), 0);
	EXPECT_EQ(memcmp(u48, &min_val, BYTES_6), 0);

	// 7 bytes
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(memcmp(u56, &max_val, BYTES_7), 0);
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, min_val), 0);
	EXPECT_EQ(memcmp(u56, &min_val, BYTES_7), 0);
}

/*
 * Insert max + 1/max/max - 1 values into 1 - 8 byte variables.
 */
TEST_F(ConverterUint, SetUintAboveBelow)
{
	// SetUp
	const uint16_t u8_above =  ((uint16_t) UINT8_MAX) + 1;
	const uint8_t  u8_below =  UINT8_MAX - 1;
	const uint32_t u16_above = ((uint32_t) UINT16_MAX) + 1;
	const uint16_t u16_below = UINT16_MAX - 1;
	const uint64_t u32_above = ((uint64_t) UINT32_MAX) + 1;
	const uint32_t u32_below = UINT32_MAX - 1;
	const uint64_t u64_below = UINT64_MAX - 1;

	const uint32_t u24_above = IPX_UINT24_MAX + 1;
	const uint32_t u24_below = IPX_UINT24_MAX - 1;
	const uint64_t u40_above = IPX_UINT40_MAX + 1;
	const uint64_t u40_below = IPX_UINT40_MAX - 1;
	const uint64_t u48_above = IPX_UINT48_MAX + 1;
	const uint64_t u48_below = IPX_UINT48_MAX - 1;
	const uint64_t u56_above = IPX_UINT56_MAX + 1;
	const uint64_t u56_below = IPX_UINT56_MAX - 1;

	// Execute
	// 1 byte
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, u8_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*u8, UINT8_MAX);  // No endian conversion needed
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, UINT8_MAX), 0);
	EXPECT_EQ(*u8, UINT8_MAX);  // No endian conversion needed
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, u8_below), 0);
	EXPECT_EQ(*u8, u8_below);   // No endian conversion needed (only 1 byte)

	// 2 bytes
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, u16_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*u16, UINT16_MAX); // No endian conversion needed
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, UINT16_MAX), 0);
	EXPECT_EQ(*u16, UINT16_MAX);  // No endian conversion needed
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, u16_below), 0);
	EXPECT_EQ(*u16, htons(u16_below));

	// 4 bytes
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, u32_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*u32, UINT32_MAX); // No endian conversion needed
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, UINT32_MAX), 0);
	EXPECT_EQ(*u32, UINT32_MAX);  // No endian conversion needed
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, u32_below), 0);
	EXPECT_EQ(*u32, htonl(u32_below));

	// 8 bytes (only the value below MAX and MAX)
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, UINT64_MAX), 0);
	EXPECT_EQ(*u64, UINT64_MAX);  // No endian conversion needed
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, u64_below), 0);
	EXPECT_EQ(*u64, htobe64(u64_below));

	// Other (unusual situations i.e. 3, 5, 6 and 7 bytes)
	const uint64_t temp_max = UINT64_MAX;
	uint8_t temp32[4];
	uint8_t temp64[8];

	// 3 bytes
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, u24_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(memcmp(u24, &temp_max, BYTES_3), 0);
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, IPX_UINT24_MAX), 0);
	EXPECT_EQ(memcmp(u24, &temp_max, BYTES_3), 0);
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, u24_below), 0);
	*((uint32_t *) temp32) = htonl(u24_below);
	EXPECT_EQ(memcmp(u24, &temp32[1], BYTES_3), 0);

	// 5 bytes
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, u40_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(memcmp(u40, &temp_max, BYTES_5), 0);
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, IPX_UINT40_MAX), 0);
	EXPECT_EQ(memcmp(u40, &temp_max, BYTES_5), 0);
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, u40_below), 0);
	*((uint64_t *) temp64) = htobe64(u40_below);
	EXPECT_EQ(memcmp(u40, &temp64[3], BYTES_5), 0);

	// 6 bytes
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, u48_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(memcmp(u48, &temp_max, BYTES_6), 0);
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, IPX_UINT48_MAX), 0);
	EXPECT_EQ(memcmp(u48, &temp_max, BYTES_6), 0);
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, u48_below), 0);
	*((uint64_t *) temp64) = htobe64(u48_below);
	EXPECT_EQ(memcmp(u48, &temp64[2], BYTES_6), 0);

	// 7 bytes
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, u56_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(memcmp(u56, &temp_max, BYTES_7), 0);
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, IPX_UINT56_MAX), 0);
	EXPECT_EQ(memcmp(u56, &temp_max, BYTES_7), 0);
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, u56_below), 0);
	*((uint64_t *) temp64) = htobe64(u56_below);
	EXPECT_EQ(memcmp(u56, &temp64[1], BYTES_7), 0);
}

/*
 * "Random" values in the valid interval for 1 - 8 bytes unsigned values
 */
TEST_F(ConverterUint, SetUintInRandom)
{
	// 1 byte
	const uint8_t u8_rand1 =  12U;
	const uint8_t u8_rand2 =  93U;
	const uint8_t u8_rand3 = 112U;
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, u8_rand1), 0);
	EXPECT_EQ(*u8, u8_rand1);
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, u8_rand2), 0);
	EXPECT_EQ(*u8, u8_rand2);
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, u8_rand3), 0);
	EXPECT_EQ(*u8, u8_rand3);

	// 2 bytes
	const uint16_t u16_rand1 =  1342U;
	const uint16_t u16_rand2 = 25432U;
	const uint16_t u16_rand3 = 45391U;
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, u16_rand1), 0);
	EXPECT_EQ(*u16, htons(u16_rand1));
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, u16_rand2), 0);
	EXPECT_EQ(*u16, htons(u16_rand2));
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, u16_rand3), 0);
	EXPECT_EQ(*u16, htons(u16_rand3));

	// 4 bytes
	const uint32_t u32_rand1 =      50832UL;
	const uint32_t u32_rand2 =   11370824UL;
	const uint32_t u32_rand3 = 3793805425UL;
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, u32_rand1), 0);
	EXPECT_EQ(*u32, htonl(u32_rand1));
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, u32_rand2), 0);
	EXPECT_EQ(*u32, htonl(u32_rand2));
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, u32_rand3), 0);
	EXPECT_EQ(*u32, htonl(u32_rand3));

	// 8 bytes
	const uint64_t u64_rand1 =         428760872517ULL;
	const uint64_t u64_rand2 =     8275792237734210ULL;
	const uint64_t u64_rand3 = 17326724161708531625ULL;
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, u64_rand1), 0);
	EXPECT_EQ(*u64, htobe64(u64_rand1));
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, u64_rand2), 0);
	EXPECT_EQ(*u64, htobe64(u64_rand2));
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, u64_rand3), 0);
	EXPECT_EQ(*u64, htobe64(u64_rand3));

	// Other (unusual situations i.e. 3, 5, 6 and 7 bytes)
	uint8_t temp32[4];
	uint8_t temp64[8];

	// 3 bytes
	const uint32_t u24_rand1 =    22311UL;
	const uint32_t u24_rand2 =   861354UL;
	const uint32_t u24_rand3 = 14075499UL;
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, u24_rand1), 0);  // Rand 1
	*((uint32_t *) temp32) = htonl(u24_rand1);
	EXPECT_EQ(memcmp(u24, &temp32[1], BYTES_3), 0);
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, u24_rand2), 0);  // Rand 2
	*((uint32_t *) temp32) = htonl(u24_rand2);
	EXPECT_EQ(memcmp(u24, &temp32[1], BYTES_3), 0);
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, u24_rand3), 0);  // Rand 3
	*((uint32_t *) temp32) = htonl(u24_rand3);
	EXPECT_EQ(memcmp(u24, &temp32[1], BYTES_3), 0);

	// 5 bytes
	const uint64_t u40_rand1 =       360214ULL;
	const uint64_t u40_rand2 =    240285687ULL;
	const uint64_t u40_rand3 = 796219095503ULL;
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, u40_rand1), 0); // Rand 1
	*((uint64_t *) temp64) = htobe64(u40_rand1);
	EXPECT_EQ(memcmp(u40, &temp64[3], BYTES_5), 0);
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, u40_rand2), 0); // Rand 2
	*((uint64_t *) temp64) = htobe64(u40_rand2);
	EXPECT_EQ(memcmp(u40, &temp64[3], BYTES_5), 0);
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, u40_rand3), 0); // Rand 3
	*((uint64_t *) temp64) = htobe64(u40_rand3);
	EXPECT_EQ(memcmp(u40, &temp64[3], BYTES_5), 0);

	// 6 bytes
	const uint64_t u48_rand1 =       696468180ULL;
	const uint64_t u48_rand2 =    671963163167ULL;
	const uint64_t u48_rand3 = 209841476899288ULL;
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, u48_rand1), 0); // Rand 1
	*((uint64_t *) temp64) = htobe64(u48_rand1);
	EXPECT_EQ(memcmp(u48, &temp64[2], BYTES_6), 0);
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, u48_rand2), 0); // Rand 2
	*((uint64_t *) temp64) = htobe64(u48_rand2);
	EXPECT_EQ(memcmp(u48, &temp64[2], BYTES_6), 0);
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, u48_rand3), 0); // Rand 3
	*((uint64_t *) temp64) = htobe64(u48_rand3);
	EXPECT_EQ(memcmp(u48, &temp64[2], BYTES_6), 0);

	// 7 bytes
	const uint64_t u56_rand1 =      194728764120ULL;
	const uint64_t u56_rand2 =   128273048983421ULL;
	const uint64_t u56_rand3 = 66086893994497342ULL;
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, u56_rand1), 0); // Rand 1
	*((uint64_t *) temp64) = htobe64(u56_rand1);
	EXPECT_EQ(memcmp(u56, &temp64[1], BYTES_7), 0);
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, u56_rand2), 0); // Rand 2
	*((uint64_t *) temp64) = htobe64(u56_rand2);
	EXPECT_EQ(memcmp(u56, &temp64[1], BYTES_7), 0);
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, u56_rand3), 0); // Rand 3
	*((uint64_t *) temp64) = htobe64(u56_rand3);
	EXPECT_EQ(memcmp(u56, &temp64[1], BYTES_7), 0);
}


/*
 * Test unsupported size of data fields
 */
TEST_F(ConverterUint, SetUintOutOfRange)
{
	const uint64_t value = 123456ULL; // Just random number

	// Just random sizes of arrays
	const size_t temp72_size =   9;
	const size_t temp88_size =  11;
	const size_t temp128_size = 16;
	const size_t temp192_size = 24;
	const size_t temp256_size = 32;

	uint8_t temp72[temp72_size];
	uint8_t temp88[temp88_size];
	uint8_t temp128[temp128_size];
	uint8_t temp192[temp192_size];
	uint8_t temp256[temp256_size];

	EXPECT_EQ(ipx_set_uint(temp72, 0, value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_set_uint(temp72, temp72_size, value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_set_uint(temp88, temp88_size, value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_set_uint(temp128, temp128_size, value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_set_uint(temp192, temp192_size, value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_set_uint(temp256, temp256_size, value), IPX_CONVERT_ERR_ARG);
}

/*
 * Test getter for maximum and minimum value
 */
TEST_F(ConverterUint, GetUintMaxMin)
{
	uint64_t conv_res;

	// 1 byte
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, UINT8_MAX), 0); // Max
	EXPECT_EQ(ipx_get_uint(u8, BYTES_1, &conv_res), 0);
	EXPECT_EQ(conv_res, (uint8_t) UINT8_MAX);

	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, 0), 0); // Min
	EXPECT_EQ(ipx_get_uint(u8, BYTES_1, &conv_res), 0);
	EXPECT_EQ(conv_res, 0U);

	// 2 bytes
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, UINT16_MAX), 0); // Max
	EXPECT_EQ(ipx_get_uint(u16, BYTES_2, &conv_res), 0);
	EXPECT_EQ(conv_res, (uint16_t) UINT16_MAX);

	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, 0), 0); // Min
	EXPECT_EQ(ipx_get_uint(u16, BYTES_2, &conv_res), 0);
	EXPECT_EQ(conv_res, 0U);

	// 4 bytes
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, UINT32_MAX), 0); // Max
	EXPECT_EQ(ipx_get_uint(u32, BYTES_4, &conv_res), 0);
	EXPECT_EQ(conv_res, (uint32_t) UINT32_MAX);

	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, 0), 0); // Min
	EXPECT_EQ(ipx_get_uint(u32, BYTES_4, &conv_res), 0);
	EXPECT_EQ(conv_res, 0U);

	// 8 bytes
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, UINT64_MAX), 0); // Max
	EXPECT_EQ(ipx_get_uint(u64, BYTES_8, &conv_res), 0);
	EXPECT_EQ(conv_res, (uint64_t) UINT64_MAX);

	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, 0), 0); // Min
	EXPECT_EQ(ipx_get_uint(u64, BYTES_8, &conv_res), 0);
	EXPECT_EQ(conv_res, 0U);

	// Other (unusual situations i.e. 3, 5, 6 and 7 bytes)
	// 3 bytes
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, IPX_UINT24_MAX), 0); // Max
	EXPECT_EQ(ipx_get_uint(u24, BYTES_3, &conv_res), 0);
	EXPECT_EQ(conv_res, IPX_UINT24_MAX);

	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, 0), 0); // Min
	EXPECT_EQ(ipx_get_uint(u24, BYTES_3, &conv_res), 0);
	EXPECT_EQ(conv_res, 0U);

	// 5 bytes
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, IPX_UINT40_MAX), 0); // Max
	EXPECT_EQ(ipx_get_uint(u40, BYTES_5, &conv_res), 0);
	EXPECT_EQ(conv_res, IPX_UINT40_MAX);

	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, 0), 0); // Min
	EXPECT_EQ(ipx_get_uint(u40, BYTES_5, &conv_res), 0);
	EXPECT_EQ(conv_res, 0U);

	// 6 bytes
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, IPX_UINT48_MAX), 0); // Max
	EXPECT_EQ(ipx_get_uint(u48, BYTES_6, &conv_res), 0);
	EXPECT_EQ(conv_res, IPX_UINT48_MAX);

	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, 0), 0); // Min
	EXPECT_EQ(ipx_get_uint(u48, BYTES_6, &conv_res), 0);
	EXPECT_EQ(conv_res, 0U);

	// 7 bytes
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, IPX_UINT56_MAX), 0); // Max
	EXPECT_EQ(ipx_get_uint(u56, BYTES_7, &conv_res), 0);
	EXPECT_EQ(conv_res, IPX_UINT56_MAX);

	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, 0), 0); // Min
	EXPECT_EQ(ipx_get_uint(u56, BYTES_7, &conv_res), 0);
	EXPECT_EQ(conv_res, 0U);
}

TEST_F(ConverterUint, GetUintRandom)
{
	uint64_t conv_res;

	// 1 byte
	const uint8_t u8_rand1 =  53U;
	const uint8_t u8_rand2 =  67U;
	const uint8_t u8_rand3 = 123U;
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, u8_rand1), 0); // Rand 1
	EXPECT_EQ(ipx_get_uint(u8, BYTES_1, &conv_res), 0);
	EXPECT_EQ(conv_res, u8_rand1);
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, u8_rand2), 0); // Rand 2
	EXPECT_EQ(ipx_get_uint(u8, BYTES_1, &conv_res), 0);
	EXPECT_EQ(conv_res, u8_rand2);
	EXPECT_EQ(ipx_set_uint(u8, BYTES_1, u8_rand3), 0); // Rand 3
	EXPECT_EQ(ipx_get_uint(u8, BYTES_1, &conv_res), 0);
	EXPECT_EQ(conv_res, u8_rand3);

	// 2 bytes
	const uint16_t u16_rand1 =   421U;
	const uint16_t u16_rand2 =  2471U;
	const uint16_t u16_rand3 = 37245U;
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, u16_rand1), 0); // Rand 1
	EXPECT_EQ(ipx_get_uint(u16, BYTES_2, &conv_res), 0);
	EXPECT_EQ(conv_res, u16_rand1);
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, u16_rand2), 0); // Rand 2
	EXPECT_EQ(ipx_get_uint(u16, BYTES_2, &conv_res), 0);
	EXPECT_EQ(conv_res, u16_rand2);
	EXPECT_EQ(ipx_set_uint(u16, BYTES_2, u16_rand3), 0); // Rand 3
	EXPECT_EQ(ipx_get_uint(u16, BYTES_2, &conv_res), 0);
	EXPECT_EQ(conv_res, u16_rand3);

	// 4 bytes
	const uint32_t u32_rand1 =     109127UL;
	const uint32_t u32_rand2 =   28947291UL;
	const uint32_t u32_rand3 = 1975298731UL;
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, u32_rand1), 0); // Rand 1
	EXPECT_EQ(ipx_get_uint(u32, BYTES_4, &conv_res), 0);
	EXPECT_EQ(conv_res, u32_rand1);
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, u32_rand2), 0); // Rand 2
	EXPECT_EQ(ipx_get_uint(u32, BYTES_4, &conv_res), 0);
	EXPECT_EQ(conv_res, u32_rand2);
	EXPECT_EQ(ipx_set_uint(u32, BYTES_4, u32_rand3), 0); // Rand 3
	EXPECT_EQ(ipx_get_uint(u32, BYTES_4, &conv_res), 0);
	EXPECT_EQ(conv_res, u32_rand3);

	// 8 bytes
	const uint64_t u64_rand1 =         147984727321ULL;
	const uint64_t u64_rand2 =     2876987613687162ULL;
	const uint64_t u64_rand3 = 11298373761876598719ULL;
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, u64_rand1), 0); // Rand 1
	EXPECT_EQ(ipx_get_uint(u64, BYTES_8, &conv_res), 0);
	EXPECT_EQ(conv_res, u64_rand1);
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, u64_rand2), 0); // Rand 2
	EXPECT_EQ(ipx_get_uint(u64, BYTES_8, &conv_res), 0);
	EXPECT_EQ(conv_res, u64_rand2);
	EXPECT_EQ(ipx_set_uint(u64, BYTES_8, u64_rand3), 0); // Rand 3
	EXPECT_EQ(ipx_get_uint(u64, BYTES_8, &conv_res), 0);
	EXPECT_EQ(conv_res, u64_rand3);

	// Other (unusual situations i.e. 3, 5, 6 and 7 bytes)
	// 3 bytes
	const uint32_t u24_rand1 =    38276UL;
	const uint32_t u24_rand2 =   763547UL;
	const uint32_t u24_rand3 = 11287321UL;
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, u24_rand1), 0); // Rand 1
	EXPECT_EQ(ipx_get_uint(u24, BYTES_3, &conv_res), 0);
	EXPECT_EQ(conv_res, u24_rand1);
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, u24_rand2), 0); // Rand 2
	EXPECT_EQ(ipx_get_uint(u24, BYTES_3, &conv_res), 0);
	EXPECT_EQ(conv_res, u24_rand2);
	EXPECT_EQ(ipx_set_uint(u24, BYTES_3, u24_rand3), 0); // Rand 3
	EXPECT_EQ(ipx_get_uint(u24, BYTES_3, &conv_res), 0);
	EXPECT_EQ(conv_res, u24_rand3);

	// 5 bytes
	const uint64_t u40_rand1 =       278632ULL;
	const uint64_t u40_rand2 =    287638124ULL;
	const uint64_t u40_rand3 = 527836261240ULL;
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, u40_rand1), 0); // Rand 1
	EXPECT_EQ(ipx_get_uint(u40, BYTES_5, &conv_res), 0);
	EXPECT_EQ(conv_res, u40_rand1);
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, u40_rand2), 0); // Rand 2
	EXPECT_EQ(ipx_get_uint(u40, BYTES_5, &conv_res), 0);
	EXPECT_EQ(conv_res, u40_rand2);
	EXPECT_EQ(ipx_set_uint(u40, BYTES_5, u40_rand3), 0); // Rand 3
	EXPECT_EQ(ipx_get_uint(u40, BYTES_5, &conv_res), 0);
	EXPECT_EQ(conv_res, u40_rand3);

	// 6 bytes
	const uint64_t u48_rand1 =       287468172ULL;
	const uint64_t u48_rand2 =    897287628371ULL;
	const uint64_t u48_rand3 = 219879286827632ULL;
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, u48_rand1), 0); // Rand 1
	EXPECT_EQ(ipx_get_uint(u48, BYTES_6, &conv_res), 0);
	EXPECT_EQ(conv_res, u48_rand1);
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, u48_rand2), 0); // Rand 2
	EXPECT_EQ(ipx_get_uint(u48, BYTES_6, &conv_res), 0);
	EXPECT_EQ(conv_res, u48_rand2);
	EXPECT_EQ(ipx_set_uint(u48, BYTES_6, u48_rand3), 0); // Rand 3
	EXPECT_EQ(ipx_get_uint(u48, BYTES_6, &conv_res), 0);
	EXPECT_EQ(conv_res, u48_rand3);

	// 7 bytes
	const uint64_t u56_rand1 =      387648182713ULL;
	const uint64_t u56_rand2 =   258628761274610ULL;
	const uint64_t u56_rand3 = 58762617654765176ULL;
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, u56_rand1), 0); // Rand 1
	EXPECT_EQ(ipx_get_uint(u56, BYTES_7, &conv_res), 0);
	EXPECT_EQ(conv_res, u56_rand1);
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, u56_rand2), 0); // Rand 2
	EXPECT_EQ(ipx_get_uint(u56, BYTES_7, &conv_res), 0);
	EXPECT_EQ(conv_res, u56_rand2);
	EXPECT_EQ(ipx_set_uint(u56, BYTES_7, u56_rand3), 0); // Rand 3
	EXPECT_EQ(ipx_get_uint(u56, BYTES_7, &conv_res), 0);
	EXPECT_EQ(conv_res, u56_rand3);
}

/*
 * Test unsupported size of data fields
 */
TEST_F(ConverterUint, GetUintOutOfRange)
{
	uint64_t value = 123456ULL; // Just random number

	// Just random sizes of arrays
	const size_t temp72_size =   9;
	const size_t temp88_size =  11;
	const size_t temp128_size = 16;
	const size_t temp192_size = 24;
	const size_t temp256_size = 32;

	uint8_t temp72[temp72_size];
	uint8_t temp88[temp88_size];
	uint8_t temp128[temp128_size];
	uint8_t temp192[temp192_size];
	uint8_t temp256[temp256_size];

	EXPECT_EQ(ipx_get_uint(temp72, 0U, &value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_get_uint(temp72, temp72_size, &value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_get_uint(temp88, temp88_size, &value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_get_uint(temp128, temp128_size, &value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_get_uint(temp192, temp192_size, &value), IPX_CONVERT_ERR_ARG);
	EXPECT_EQ(ipx_get_uint(temp256, temp256_size, &value), IPX_CONVERT_ERR_ARG);
}

/**
 * \brief Test fixture for SetUint tests
 */
class ConverterInt : public ::testing::Test {
protected:
	/*
	 * We want to have all variables dynamically allocated so Valgrind can check
	 * access out of bounds, etc.
     */
	int8_t  *i8;
	int16_t *i16;
	int32_t *i32;
	int64_t *i64;

	int8_t *i24;
	int8_t *i40;
	int8_t *i48;
	int8_t *i56;

public:
	/** Create variables for tests */
	virtual void SetUp() {
		i8  = new int8_t;
		i16 = new int16_t;
		i32 = new int32_t;
		i64 = new int64_t;

		i24 = new int8_t[3];
		i40 = new int8_t[5];
		i48 = new int8_t[6];
		i56 = new int8_t[7];
	}

	/** Destroy variables for the tests */
	virtual void TearDown() {
		delete i8;
		delete i16;
		delete i32;
		delete i64;

		delete[] i24;
		delete[] i40;
		delete[] i48;
		delete[] i56;
	}
};

/*
 * Insert the maximum possible value i.e. "INT64_MAX" and the minimum possible
 * value i.e. "INT64_MIN" into 1 - 8 byte variables. The test expects
 * truncation of values.
 */
TEST_F(ConverterInt, SetIntMaxMin) {
	// SetUp
	const int64_t max_val = INT64_MAX;
	const int64_t min_val = INT64_MIN;

	// Execute
	// 1 byte
	EXPECT_EQ(ipx_set_int(i8, BYTES_1, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i8, INT8_MAX);
	EXPECT_EQ(ipx_set_int(i8, BYTES_1, min_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i8, INT8_MIN);

	// 2 bytes
	EXPECT_EQ(ipx_set_int(i16, BYTES_2, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i16, (int16_t) htons(INT16_MAX));
	EXPECT_EQ(ipx_set_int(i16, BYTES_2, min_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i16, (int16_t) htons(INT16_MIN));

	// 4 bytes
	EXPECT_EQ(ipx_set_int(i32, BYTES_4, max_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i32, (int32_t) htonl(INT32_MAX));
	EXPECT_EQ(ipx_set_int(i32, BYTES_4, min_val), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i32, (int32_t) htonl(INT32_MIN));

	// 8 bytes
	EXPECT_EQ(ipx_set_int(i64, BYTES_8, max_val), 0);
	EXPECT_EQ(*i64, (int64_t) htobe64(INT64_MAX));
	EXPECT_EQ(ipx_set_int(i64, BYTES_8, min_val), 0);
	EXPECT_EQ(*i64, (int64_t) htobe64(INT64_MIN));

	// Other (unusual situations i.e. 3, 5, 6 and 7 bytes)
	int8_t temp32[4];
	int8_t temp64[8];

	// 3 bytes
	EXPECT_EQ(ipx_set_int(i24, BYTES_3, max_val), IPX_CONVERT_ERR_TRUNC);
	*((uint32_t *) temp32) = htonl(IPX_INT24_MAX);
	EXPECT_EQ(memcmp(i24, &temp32[1], BYTES_3), 0);
	EXPECT_EQ(ipx_set_int(i24, BYTES_3, min_val), IPX_CONVERT_ERR_TRUNC);
	*((uint32_t *) temp32) = htonl(IPX_INT24_MIN);
	EXPECT_EQ(memcmp(i24, &temp32[1], BYTES_3), 0);

	// 5 bytes
	EXPECT_EQ(ipx_set_int(i40, BYTES_5, max_val), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT40_MAX);
	EXPECT_EQ(memcmp(i40, &temp64[3], BYTES_5), 0);
	EXPECT_EQ(ipx_set_int(i40, BYTES_5, min_val), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT40_MIN);
	EXPECT_EQ(memcmp(i40, &temp64[3], BYTES_5), 0);

	// 6 bytes
	EXPECT_EQ(ipx_set_int(i48, BYTES_6, max_val), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT48_MAX);
	EXPECT_EQ(memcmp(i48, &temp64[2], BYTES_6), 0);
	EXPECT_EQ(ipx_set_int(i48, BYTES_6, min_val), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT48_MIN);
	EXPECT_EQ(memcmp(i48, &temp64[2], BYTES_6), 0);

	// 7 bytes
	EXPECT_EQ(ipx_set_int(i56, BYTES_7, max_val), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT56_MAX);
	EXPECT_EQ(memcmp(i56, &temp64[1], BYTES_7), 0);
	EXPECT_EQ(ipx_set_int(i56, BYTES_7, min_val), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT56_MIN);
	EXPECT_EQ(memcmp(i56, &temp64[1], BYTES_7), 0);
}

/*
 * Insert max + 1/max/max - 1 and min - 1/min/min + 1 values into 1 - 8 byte
 * variables.
 */
TEST_F(ConverterInt, SetIntAboveBelow)
{
	// SetUp
	const int16_t i8_max_above  = ((int16_t) INT8_MAX) + 1;
	const int8_t  i8_max_below  = INT8_MAX - 1;
	const int32_t i16_max_above = ((int32_t) INT16_MAX) + 1;
	const int16_t i16_max_below = INT16_MAX - 1;
	const int64_t i32_max_above = ((int64_t) INT32_MAX) + 1;
	const int32_t i32_max_below = INT32_MAX - 1;
	const int64_t i64_max_below = INT64_MAX - 1;

	const int32_t i24_max_above = IPX_INT24_MAX + 1;
	const int32_t i24_max_below = IPX_INT24_MAX - 1;
	const int64_t i40_max_above = IPX_INT40_MAX + 1;
	const int64_t i40_max_below = IPX_INT40_MAX - 1;
	const int64_t i48_max_above = IPX_INT48_MAX + 1;
	const int64_t i48_max_below = IPX_INT48_MAX - 1;
	const int64_t i56_max_above = IPX_INT56_MAX + 1;
	const int64_t i56_max_below = IPX_INT56_MAX - 1;

	const int8_t  i8_min_above  = INT8_MIN + 1;
	const int16_t i8_min_below  = ((int16_t) INT8_MIN) - 1;
	const int16_t i16_min_above = INT16_MIN + 1;
	const int32_t i16_min_below = ((int32_t) INT16_MIN) - 1;
	const int32_t i32_min_above = INT32_MIN + 1;
	const int64_t i32_min_below = ((int64_t) INT32_MIN) - 1;
	const int64_t i64_min_above = INT64_MIN + 1;

	const int32_t i24_min_above = IPX_INT24_MIN + 1;
	const int32_t i24_min_below = IPX_INT24_MIN - 1;
	const int64_t i40_min_above = IPX_INT40_MIN + 1;
	const int64_t i40_min_below = IPX_INT40_MIN - 1;
	const int64_t i48_min_above = IPX_INT48_MIN + 1;
	const int64_t i48_min_below = IPX_INT48_MIN - 1;
	const int64_t i56_min_above = IPX_INT56_MIN + 1;
	const int64_t i56_min_below = IPX_INT56_MIN - 1;

	// Execute
	// 1 byte
	EXPECT_EQ(ipx_set_int(i8, BYTES_1, i8_max_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i8, INT8_MAX);
	EXPECT_EQ(ipx_set_int(i8, BYTES_1, INT8_MAX), 0);
	EXPECT_EQ(*i8, INT8_MAX);
	EXPECT_EQ(ipx_set_int(i8, BYTES_1, i8_max_below), 0);
	EXPECT_EQ(*i8, i8_max_below);

	EXPECT_EQ(ipx_set_int(i8, BYTES_1, i8_min_above), 0);
	EXPECT_EQ(*i8, i8_min_above);
	EXPECT_EQ(ipx_set_int(i8, BYTES_1, INT8_MIN), 0);
	EXPECT_EQ(*i8, INT8_MIN);
	EXPECT_EQ(ipx_set_int(i8, BYTES_1, i8_min_below), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i8, INT8_MIN);

	// 2 bytes
	EXPECT_EQ(ipx_set_int(i16, BYTES_2, i16_max_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i16, (int16_t) htons(INT16_MAX));
	EXPECT_EQ(ipx_set_int(i16, BYTES_2, INT16_MAX), 0);
	EXPECT_EQ(*i16, (int16_t) htons(INT16_MAX));
	EXPECT_EQ(ipx_set_int(i16, BYTES_2, i16_max_below), 0);
	EXPECT_EQ(*i16, (int16_t) htons(i16_max_below));

	EXPECT_EQ(ipx_set_int(i16, BYTES_2, i16_min_above), 0);
	EXPECT_EQ(*i16, (int16_t) htons(i16_min_above));
	EXPECT_EQ(ipx_set_int(i16, BYTES_2, INT16_MIN), 0);
	EXPECT_EQ(*i16, (int16_t) htons(INT16_MIN));
	EXPECT_EQ(ipx_set_int(i16, BYTES_2, i16_min_below), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i16, (int16_t) htons(INT16_MIN));

	// 4 bytes
	EXPECT_EQ(ipx_set_int(i32, BYTES_4, i32_max_above), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i32, (int32_t) htonl(INT32_MAX));
	EXPECT_EQ(ipx_set_int(i32, BYTES_4, INT32_MAX), 0);
	EXPECT_EQ(*i32, (int32_t) htonl(INT32_MAX));
	EXPECT_EQ(ipx_set_int(i32, BYTES_4, i32_max_below), 0);
	EXPECT_EQ(*i32, (int32_t) htonl(i32_max_below));

	EXPECT_EQ(ipx_set_int(i32, BYTES_4, i32_min_above), 0);
	EXPECT_EQ(*i32, (int32_t) htonl(i32_min_above));
	EXPECT_EQ(ipx_set_int(i32, BYTES_4, INT32_MIN), 0);
	EXPECT_EQ(*i32, (int32_t) htonl(INT32_MIN));
	EXPECT_EQ(ipx_set_int(i32, BYTES_4, i32_min_below), IPX_CONVERT_ERR_TRUNC);
	EXPECT_EQ(*i32, (int32_t) htonl(INT32_MIN));

	// 4 bytes
	EXPECT_EQ(ipx_set_int(i64, BYTES_8, INT64_MAX), 0);
	EXPECT_EQ(*i64, (int64_t) htobe64(INT64_MAX));
	EXPECT_EQ(ipx_set_int(i64, BYTES_8, i64_max_below), 0);
	EXPECT_EQ(*i64, (int64_t) htobe64(i64_max_below));

	EXPECT_EQ(ipx_set_int(i64, BYTES_8, i64_min_above), 0);
	EXPECT_EQ(*i64, (int64_t) htobe64(i64_min_above));
	EXPECT_EQ(ipx_set_int(i64, BYTES_8, INT64_MIN), 0);
	EXPECT_EQ(*i64, (int64_t) htobe64(INT64_MIN));

	// Other (unusual situations i.e. 3, 5, 6 and 7 bytes)
	int8_t temp32[4];
	int8_t temp64[8];

	// 3 bytes
	EXPECT_EQ(ipx_set_int(i24, BYTES_3, i24_max_above), IPX_CONVERT_ERR_TRUNC);
	*((uint32_t *) temp32) = htonl(IPX_INT24_MAX);
	EXPECT_EQ(memcmp(i24, &temp32[1], BYTES_3), 0);
	EXPECT_EQ(ipx_set_int(i24, BYTES_3, IPX_INT24_MAX), 0);
	*((uint32_t *) temp32) = htonl(IPX_INT24_MAX);
	EXPECT_EQ(memcmp(i24, &temp32[1], BYTES_3), 0);
	EXPECT_EQ(ipx_set_int(i24, BYTES_3, i24_max_below), 0);
	*((uint32_t *) temp32) = htonl(i24_max_below);
	EXPECT_EQ(memcmp(i24, &temp32[1], BYTES_3), 0);

	EXPECT_EQ(ipx_set_int(i24, BYTES_3, i24_min_above), 0);
	*((uint32_t *) temp32) = htonl(i24_min_above);
	EXPECT_EQ(memcmp(i24, &temp32[1], BYTES_3), 0);
	EXPECT_EQ(ipx_set_int(i24, BYTES_3, IPX_INT24_MIN), 0);
	*((uint32_t *) temp32) = htonl(IPX_INT24_MIN);
	EXPECT_EQ(memcmp(i24, &temp32[1], BYTES_3), 0);
	EXPECT_EQ(ipx_set_int(i24, BYTES_3, i24_min_below), IPX_CONVERT_ERR_TRUNC);
	*((uint32_t *) temp32) = htonl(IPX_INT24_MIN);
	EXPECT_EQ(memcmp(i24, &temp32[1], BYTES_3), 0);

	// 5 bytes
	EXPECT_EQ(ipx_set_int(i40, BYTES_5, i40_max_above), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT40_MAX);
	EXPECT_EQ(memcmp(i40, &temp64[3], BYTES_5), 0);
	EXPECT_EQ(ipx_set_int(i40, BYTES_5, IPX_INT40_MAX), 0);
	*((uint64_t *) temp64) = htobe64(IPX_INT40_MAX);
	EXPECT_EQ(memcmp(i40, &temp64[3], BYTES_5), 0);
	EXPECT_EQ(ipx_set_int(i40, BYTES_5, i40_max_below), 0);
	*((uint64_t *) temp64) = htobe64(i40_max_below);
	EXPECT_EQ(memcmp(i40, &temp64[3], BYTES_5), 0);

	EXPECT_EQ(ipx_set_int(i40, BYTES_5, i40_min_above), 0);
	*((uint64_t *) temp64) = htobe64(i40_min_above);
	EXPECT_EQ(memcmp(i40, &temp64[3], BYTES_5), 0);
	EXPECT_EQ(ipx_set_int(i40, BYTES_5, IPX_INT40_MIN), 0);
	*((uint64_t *) temp64) = htobe64(IPX_INT40_MIN);
	EXPECT_EQ(memcmp(i40, &temp64[3], BYTES_5), 0);
	EXPECT_EQ(ipx_set_int(i40, BYTES_5, i40_min_below), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT40_MIN);
	EXPECT_EQ(memcmp(i40, &temp64[3], BYTES_5), 0);

	// 6 bytes
	EXPECT_EQ(ipx_set_int(i48, BYTES_6, i48_max_above), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT48_MAX);
	EXPECT_EQ(memcmp(i48, &temp64[2], BYTES_6), 0);
	EXPECT_EQ(ipx_set_int(i48, BYTES_6, IPX_INT48_MAX), 0);
	*((uint64_t *) temp64) = htobe64(IPX_INT48_MAX);
	EXPECT_EQ(memcmp(i48, &temp64[2], BYTES_6), 0);
	EXPECT_EQ(ipx_set_int(i48, BYTES_6, i48_max_below), 0);
	*((uint64_t *) temp64) = htobe64(i48_max_below);
	EXPECT_EQ(memcmp(i48, &temp64[2], BYTES_6), 0);

	EXPECT_EQ(ipx_set_int(i48, BYTES_6, i48_min_above), 0);
	*((uint64_t *) temp64) = htobe64(i48_min_above);
	EXPECT_EQ(memcmp(i48, &temp64[2], BYTES_6), 0);
	EXPECT_EQ(ipx_set_int(i48, BYTES_6, IPX_INT48_MIN), 0);
	*((uint64_t *) temp64) = htobe64(IPX_INT48_MIN);
	EXPECT_EQ(memcmp(i48, &temp64[2], BYTES_6), 0);
	EXPECT_EQ(ipx_set_int(i48, BYTES_6, i48_min_below), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT48_MIN);
	EXPECT_EQ(memcmp(i48, &temp64[2], BYTES_6), 0);

	// 7 bytes
	EXPECT_EQ(ipx_set_int(i56, BYTES_7, i56_max_above), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT56_MAX);
	EXPECT_EQ(memcmp(i56, &temp64[1], BYTES_7), 0);
	EXPECT_EQ(ipx_set_int(i56, BYTES_7, IPX_INT56_MAX), 0);
	*((uint64_t *) temp64) = htobe64(IPX_INT56_MAX);
	EXPECT_EQ(memcmp(i56, &temp64[1], BYTES_7), 0);
	EXPECT_EQ(ipx_set_int(i56, BYTES_7, i56_max_below), 0);
	*((uint64_t *) temp64) = htobe64(i56_max_below);
	EXPECT_EQ(memcmp(i56, &temp64[1], BYTES_7), 0);

	EXPECT_EQ(ipx_set_int(i56, BYTES_7, i56_min_above), 0);
	*((uint64_t *) temp64) = htobe64(i56_min_above);
	EXPECT_EQ(memcmp(i56, &temp64[1], BYTES_7), 0);
	EXPECT_EQ(ipx_set_int(i56, BYTES_7, IPX_INT56_MIN), 0);
	*((uint64_t *) temp64) = htobe64(IPX_INT56_MIN);
	EXPECT_EQ(memcmp(i56, &temp64[1], BYTES_7), 0);
	EXPECT_EQ(ipx_set_int(i56, BYTES_7, i56_min_below), IPX_CONVERT_ERR_TRUNC);
	*((uint64_t *) temp64) = htobe64(IPX_INT56_MIN);
	EXPECT_EQ(memcmp(i56, &temp64[1], BYTES_7), 0);

}

/**
 * @}
 */

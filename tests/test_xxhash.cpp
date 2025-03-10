#include <stdio.h>
#include <iostream>
#include <gtest/gtest.h>
#include <lzbench/lzbench-lib.h>

#include <xxhash.h>
#include <lzbench/lzbench-lib.h>

#  include <stdint.h>
typedef uint8_t  XSUM_U8;
typedef uint32_t XSUM_U32;
typedef uint64_t XSUM_U64;

/* use #define to make them constant, required for initialization */
#define PRIME32 2654435761U
#define PRIME64 11400714785074694797ULL


#define SANITY_BUFFER_SIZE (4096 + 64 + 1)

#include "sanity_test_vectors.h"
/* TODO : Share this function with sanity_check.c and xsum_sanity_check.c */
/*
 * Fills a test buffer with pseudorandom data.
 *
 * This is used in the sanity check - its values must not be changed.
 */
static void fillTestBuffer(XSUM_U8* buffer, size_t bufferLenInBytes)
{
    XSUM_U64 byteGen = PRIME32;
    size_t i;

    assert(buffer != NULL);

    for (i = 0; i < bufferLenInBytes; ++i) {
        buffer[i] = (XSUM_U8)(byteGen>>56);
        byteGen *= PRIME64;
    }
}


/* TODO : Share this function with sanity_check.c and xsum_sanity_check.c */
/*
 * Create (malloc) and fill buffer with pseudorandom data for sanity check.
 *
 * Use releaseSanityBuffer() to delete the buffer.
 */
static XSUM_U8* createSanityBuffer(size_t bufferLenInBytes)
{
    XSUM_U8* buffer = (XSUM_U8*) malloc(bufferLenInBytes);
    assert(buffer != NULL);
    fillTestBuffer(buffer, bufferLenInBytes);
    return buffer;
}


/* TODO : Share this function with xsum_sanity_check.c */
/**/
static void checkResult32(XXH32_hash_t r1, XXH32_hash_t r2, const char* testName, size_t testNb, size_t lineNb)
{
    ASSERT_EQ(r1, r2);

}


/* TODO : Share this function with xsum_sanity_check.c */
/**/
static void checkResult64(XXH64_hash_t r1, XXH64_hash_t r2, const char* testName, size_t testNb, size_t lineNb)
{
    ASSERT_EQ(r1, r2);
}


/* TODO : Share this function with xsum_sanity_check.c */
/**/
static void checkResult128(XXH128_hash_t r1, XXH128_hash_t r2, const char* testName, size_t testNb, size_t lineNb)
{
    ASSERT_EQ(r1.low64, r2.low64);
    ASSERT_EQ(r1.high64, r2.high64);
}


/* TODO : Share this function with xsum_sanity_check.c */
/**/
static void checkResultTestDataSample(const XSUM_U8* r1, const XSUM_U8* r2, const char* testName, size_t testNb, size_t lineNb)
{
    int cmp = memcmp(r1, r2, SECRET_SAMPLE_NBBYTES);
    ASSERT_EQ(cmp, 0);
   
}


/* TODO : Share this function with xsum_sanity_check.c */
/**/
static void testXXH32(
    const void* data,
    const XSUM_testdata32_t* testData,
    const char* testName,
    size_t testNb
) {
    size_t   const len     = testData->len;
    XSUM_U32 const seed    = testData->seed;
    XSUM_U32 const Nresult = testData->Nresult;

    XXH32_state_t * const state = XXH32_createState();

    if (len == 0) {
        data = NULL;
    } else {
        EXPECT_NE(data, nullptr);
    }

    EXPECT_NE(state, nullptr);

    checkResult32(XXH32(data, len, seed), Nresult, testName, testNb, __LINE__);

    (void)XXH32_reset(state, seed);
    (void)XXH32_update(state, data, len);
    checkResult32(XXH32_digest(state), Nresult, testName, testNb, __LINE__);

    (void)XXH32_reset(state, seed);
    {
        size_t pos;
        for (pos = 0; pos < len; ++pos) {
            (void)XXH32_update(state, ((const char*)data)+pos, 1);
        }
    }
    checkResult32(XXH32_digest(state), Nresult, testName, testNb, __LINE__);

    XXH32_freeState(state);
}

TEST(XXHASH, testXXH32)
{
    size_t testCount = 0;
    size_t      const sanityBufferSizeInBytes = SANITY_BUFFER_SIZE;
    XSUM_U8*    const sanityBuffer            = createSanityBuffer(sanityBufferSizeInBytes);
    const void* const secret                  = sanityBuffer + 7;
    size_t      const secretSize              = XXH3_SECRET_SIZE_MIN + 11;
    ASSERT_GT(sanityBufferSizeInBytes, 7 + secretSize);

    {
        /* XXH32 */
        size_t const n = sizeof(XSUM_XXH32_testdata) / sizeof(XSUM_XXH32_testdata[0]);
        size_t i;
        for (i = 0; i < n; ++i, ++testCount) {
            testXXH32(
                sanityBuffer,
                &XSUM_XXH32_testdata[i],
                "XSUM_XXH32_testdata",
                i
            );
        }
    }
}

int main (int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
/*
Copyright 2011-2024 Frederic Langlet
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
you may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <algorithm>
#include <cstring>

#include "RLT.hpp"
#include "../Global.hpp"


using namespace kanzi;
using namespace std;

bool RLT::forward(SliceArray<byte>& input, SliceArray<byte>& output, int count)
{
    if (count == 0)
        return true;

    if (count < MIN_BLOCK_LENGTH)
        return false;

    if (!SliceArray<byte>::isValid(input))
        throw invalid_argument("RLT: Invalid input block");

    if (!SliceArray<byte>::isValid(output))
        throw invalid_argument("RLT: Invalid output block");

    if (output._length - output._index < getMaxEncodedLength(count))
        return false;

    const byte* src = &input._array[input._index];
    byte* dst = &output._array[output._index];
    Global::DataType dt = Global::UNDEFINED;
    bool findBestEscape = true;

    if (_pCtx != nullptr) {
        dt = (Global::DataType) _pCtx->getInt("dataType", Global::UNDEFINED);

        if ((dt == Global::DNA) || (dt == Global::BASE64) || (dt == Global::UTF8))
            return false;

        std::string entropyType = _pCtx->getString("entropy");
        transform(entropyType.begin(), entropyType.end(), entropyType.begin(), ::toupper);

        // Fast track if fast entropy coder is used
        if ((entropyType == "NONE") || (entropyType == "ANS0") ||
            (entropyType == "HUFFMAN") || (entropyType == "RANGE"))
            findBestEscape = false;
    }

    byte escape = DEFAULT_ESCAPE;

    if (findBestEscape == true) {
        uint freqs[256] = { 0 };
        Global::computeHistogram(&src[0], count, freqs);

        if (dt == Global::UNDEFINED) {
            dt = Global::detectSimpleType(count, freqs);

            if ((_pCtx != nullptr) && (dt != Global::UNDEFINED))
                _pCtx->putInt("dataType", dt);

            if ((dt == Global::DNA) || (dt == Global::BASE64) || (dt == Global::UTF8))
                return false;
        }

        int minIdx = 0;

        if (freqs[minIdx] > 0) {
            for (int i = 1; i < 256; i++) {
                if (freqs[i] < freqs[minIdx]) {
                    minIdx = i;

                    if (freqs[i] == 0)
                        break;
                }
            }
        }

        escape = byte(minIdx);
    }

    int srcIdx = 0;
    int dstIdx = 0;
    const int srcEnd = count;
    const int srcEnd4 = srcEnd - 4;
    const int dstEnd = output._length;
    bool res = true;
    int run = 0;
    byte prev = src[srcIdx++];
    dst[dstIdx++] = escape;
    dst[dstIdx++] = prev;

    if (prev == escape)
        dst[dstIdx++] = byte(0);

    // Main loop
    while (true) {
        if (prev == src[srcIdx]) {
            const uint32 v = 0x01010101 * uint32(prev);

            if (memcmp(&v, &src[srcIdx], 4) == 0) {
                srcIdx += 4; run += 4;

                if ((run < MAX_RUN4) && (srcIdx < srcEnd4))
                    continue;
            }
            else if (prev == src[srcIdx]) {
                srcIdx++; run++;

                if (prev == src[srcIdx]) {
                    srcIdx++; run++;

                    if (prev == src[srcIdx]) {
                        srcIdx++; run++;

                        if ((run < MAX_RUN4) && (srcIdx < srcEnd4))
                            continue;
                    }
                }
            }
        }

        if (run > RUN_THRESHOLD) {
            if (dstIdx + 6 >= dstEnd) {
                res = false;
                break;
            }

            dstIdx += emitRunLength(&dst[dstIdx], run, escape, prev);
        }
        else if (prev != escape) {
            if (dstIdx + run >= dstEnd) {
               res = false;
               break;
            }

            if (run-- > 0)
               dst[dstIdx++] = prev;

            while (run-- > 0)
               dst[dstIdx++] = prev;
        }
        else { // escape literal
            if (dstIdx + (2 * run) >= dstEnd) {
               res = false;
               break;
            }

            while (run-- > 0) {
               dst[dstIdx++] = escape;
               dst[dstIdx++] = byte(0);
            }
        }

        prev = src[srcIdx];
        srcIdx++;
        run = 1;

        if (srcIdx >= srcEnd4)
            break;
    }

    if (res == true) {
        // run == 1
        if (prev != escape) {
            if (dstIdx + run < dstEnd) {
               while (run-- > 0)
                  dst[dstIdx++] = prev;
            }
        }
        else { // escape literal
            if (dstIdx + (2 * run) < dstEnd) {
               while (run-- > 0) {
                  dst[dstIdx++] = escape;
                  dst[dstIdx++] = byte(0);
               }
            }
        }

        // Emit the last few bytes
        while ((srcIdx < srcEnd) && (dstIdx < dstEnd)) {
            if (src[srcIdx] == escape) {
               if (dstIdx + 2 >= dstEnd) {
                  res = false;
                  break;
               }

               dst[dstIdx++] = escape;
               dst[dstIdx++] = byte(0);
               srcIdx++;
               continue;
            }

            dst[dstIdx++] = src[srcIdx++];
        }

        res &= (srcIdx == srcEnd);
    }

    input._index += srcIdx;
    output._index += dstIdx;
    return res && (dstIdx < srcIdx);
}

int RLT::emitRunLength(byte dst[], int run, byte escape, byte val) {
    dst[0] = val;
    dst[1] = byte(0);
    int dstIdx = (val == escape) ? 2 : 1;
    dst[dstIdx++] = escape;
    run -= RUN_THRESHOLD;

    // Encode run length
    if (run >= RUN_LEN_ENCODE1) {
        if (run < RUN_LEN_ENCODE2) {
            run -= RUN_LEN_ENCODE1;
            dst[dstIdx++] = byte(RUN_LEN_ENCODE1 + (run >> 8));
        }
        else {
            run -= RUN_LEN_ENCODE2;
            dst[dstIdx++] = byte(0xFF);
            dst[dstIdx++] = byte(run >> 8);
        }
    }

    dst[dstIdx] = byte(run);
    return dstIdx + 1;
}

bool RLT::inverse(SliceArray<byte>& input, SliceArray<byte>& output, int count)
{
    if (count == 0)
        return true;

    if (!SliceArray<byte>::isValid(input))
        throw invalid_argument("RLT: Invalid input block");

    if (!SliceArray<byte>::isValid(output))
        throw invalid_argument("RLT: Invalid output block");

    const byte* src = &input._array[input._index];
    byte* dst = &output._array[output._index];
    int srcIdx = 0;
    int dstIdx = 0;
    const int srcEnd = srcIdx + count;
    const int dstEnd = output._length;
    bool res = true;
    const byte escape = src[srcIdx++];

    if (src[srcIdx] == escape) {
        srcIdx++;

        // The data cannot start with a run but may start with an escape literal
        if ((srcIdx < srcEnd) && (src[srcIdx] != byte(0)))
            return false;

        dst[dstIdx++] = escape;
        srcIdx++;
    }

    // Main loop
    while (srcIdx < srcEnd) {
        if (src[srcIdx] != escape) {
            // Literal
            if (dstIdx >= dstEnd) {
                res = false;
                break;
            }

            dst[dstIdx++] = src[srcIdx++];
            continue;
        }

        srcIdx++;

        if (srcIdx >= srcEnd) {
            res = false;
            break;
        }

        int run = int(src[srcIdx++]);

        if (run == 0) {
            // Just an escape symbol, not a run
            if (dstIdx >= dstEnd) {
                  res = false;
                  break;
            }

            dst[dstIdx++] = escape;
            continue;
        }

        // Decode run length
        if (run == 0xFF) {
            if (srcIdx + 1 >= srcEnd) {
                  res = false;
                  break;
            }

            run = (int(src[srcIdx]) << 8) | int(src[srcIdx + 1]);
            srcIdx += 2;
            run += RUN_LEN_ENCODE2;
        }
        else if (run >= RUN_LEN_ENCODE1) {
            if (srcIdx >= srcEnd) {
                  res = false;
                  break;
            }

            run = ((run - RUN_LEN_ENCODE1) << 8) | int(src[srcIdx]);
            srcIdx++;
            run += RUN_LEN_ENCODE1;
        }

        run += (RUN_THRESHOLD - 1);

        if ((dstIdx + run >= dstEnd) || (run > MAX_RUN)) {
            res = false;
            break;
        }

        memset(&dst[dstIdx], int(dst[dstIdx - 1]), size_t(run));
        dstIdx += run;
    }

    input._index += srcIdx;
    output._index += dstIdx;
    return res && (srcIdx == srcEnd);
}

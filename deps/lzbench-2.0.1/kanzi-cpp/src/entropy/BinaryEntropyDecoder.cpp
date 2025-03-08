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
#include <stdexcept>
#include "BinaryEntropyDecoder.hpp"
#include "../Memory.hpp"
#include "EntropyUtils.hpp"

using namespace kanzi;
using namespace std;

BinaryEntropyDecoder::BinaryEntropyDecoder(InputBitStream& bitstream, Predictor* predictor, bool deallocate)
    : _predictor(predictor)
    , _bitstream(bitstream)
    , _deallocate(deallocate)
    , _sba(new byte[0], 0)
{
    if (predictor == nullptr)
        throw invalid_argument("Invalid null predictor parameter");

    _low = 0;
    _high = TOP;
    _current = 0;
}

BinaryEntropyDecoder::~BinaryEntropyDecoder()
{
    _dispose();
    delete[] _sba._array;

    if (_deallocate)
        delete _predictor;
}

int BinaryEntropyDecoder::decode(byte block[], uint blkptr, uint count)
{
    if (count >= MAX_BLOCK_SIZE)
        throw invalid_argument("Invalid block size parameter (max is 1<<30)");

    uint startChunk = blkptr;
    const uint end = blkptr + count;
    uint length = max(count, uint(64));

    if (length >= MAX_CHUNK_SIZE) {
        // If the block is big (>=64MB), split the decoding to avoid allocating
        // too much memory.
        length = (length / 8 < MAX_CHUNK_SIZE) ? count >> 3 : count >> 4;
    }

    // Split block into chunks, read bit array from bitstream and decode chunk
    while (startChunk < end) {
        const uint chunkSize = min(length, end - startChunk);

        if (_sba._length < int(chunkSize + (chunkSize >> 3))) {
            delete[] _sba._array;
            _sba._length = int(chunkSize + (chunkSize >> 3));
            _sba._array = new byte[_sba._length];
        }

        const int szBytes = int(EntropyUtils::readVarInt(_bitstream));
        _current = _bitstream.readBits(56);

        if (szBytes != 0)
           _bitstream.readBits(&_sba._array[0], 8 * szBytes);

        _sba._index = 0;
        const uint endChunk = startChunk + chunkSize;

        for (uint i = startChunk; i < endChunk; i++) {
           block[i] = byte((decodeBit(_predictor->get()) << 7)
                         | (decodeBit(_predictor->get()) << 6)
                         | (decodeBit(_predictor->get()) << 5)
                         | (decodeBit(_predictor->get()) << 4)
                         | (decodeBit(_predictor->get()) << 3)
                         | (decodeBit(_predictor->get()) << 2)
                         | (decodeBit(_predictor->get()) << 1)
                         |  decodeBit(_predictor->get()));
        }

        startChunk = endChunk;
    }

    return count;
}


// no inline
void BinaryEntropyDecoder::read()
{
    _low = (_low << 32) & MASK_0_56;
    _high = ((_high << 32) | MASK_0_32) & MASK_0_56;
    const uint64 val = BigEndian::readInt32(&_sba._array[_sba._index]) & MASK_0_32;
    _current = ((_current << 32) | val) & MASK_0_56;
    _sba._index += 4;
}

// no inline
byte BinaryEntropyDecoder::decodeByte()
{
    return byte((decodeBit(_predictor->get()) << 7)
        | (decodeBit(_predictor->get()) << 6)
        | (decodeBit(_predictor->get()) << 5)
        | (decodeBit(_predictor->get()) << 4)
        | (decodeBit(_predictor->get()) << 3)
        | (decodeBit(_predictor->get()) << 2)
        | (decodeBit(_predictor->get()) << 1)
        |  decodeBit(_predictor->get()));
}


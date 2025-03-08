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

#include "DefaultInputBitStream.hpp"
#include "../util.hpp"
#include "../io/IOException.hpp"

using namespace kanzi;
using namespace std;

DefaultInputBitStream::DefaultInputBitStream(InputStream& is, uint bufferSize) : _is(is)
{
    if (bufferSize < 1024)
        throw invalid_argument("Invalid buffer size (must be at least 1024)");

    if (bufferSize > 1 << 29)
        throw invalid_argument("Invalid buffer size (must be at most 536870912)");

    if ((bufferSize & 7) != 0)
        throw invalid_argument("Invalid buffer size (must be a multiple of 8)");

    _bufferSize = bufferSize;
    _buffer = new byte[_bufferSize];
    _availBits = 0;
    _maxPosition = -1;
    _position = 0;
    _current = 0;
    _read = 0;
    _closed = false;
}

DefaultInputBitStream::~DefaultInputBitStream()
{
    _close();
    delete[] _buffer;
}

uint DefaultInputBitStream::readBits(byte bits[], uint count)
{
    if (isClosed() == true)
        throw BitStreamException("Stream closed", BitStreamException::STREAM_CLOSED);

    if (count == 0)
        return 0;

    uint remaining = count;
    uint start = 0;

    // Byte aligned cursor ?
    if ((_availBits & 7) == 0) {
        if (_availBits == 0)
            _availBits = pullCurrent();

        // Empty _current
        while ((_availBits > 0) && (remaining >= 8)) {
            bits[start] = byte(readBits(8));
            start++;
            remaining -= 8;
        }

        prefetchRead(&_buffer[_position]);
        uint availBytes = uint(_maxPosition + 1 - _position);

        // Copy internal buffer to bits array
        while ((remaining >> 3) > availBytes) {
            memcpy(&bits[start], &_buffer[_position], availBytes);
            start += availBytes;
            remaining -= (availBytes << 3);

            if (readFromInputStream(_bufferSize) < int(_bufferSize))
                break;

            availBytes = uint(_maxPosition + 1 - _position);
        }

        const uint r = (remaining >> 6) << 3;

        if (r > 0) {
            memcpy(&bits[start], &_buffer[_position], r);
            _position += r;
            start += r;
            remaining -= (r << 3);
        }
    }
    else if (remaining >= 64) {
        // Not byte aligned
        const uint r = 64 - _availBits;
        const uint a = _availBits;

        while (remaining >= 256) {
            const uint64 v0 = _current;

            if (_position + 32 > _maxPosition) {
                _availBits = pullCurrent() - r;
                BigEndian::writeLong64(&bits[start], (v0 << r) | (_current >> _availBits));
                start += 8;
                remaining -= 64;
                continue;
            }

            const uint64 v1 = BigEndian::readLong64(&_buffer[_position + 0]);
            const uint64 v2 = BigEndian::readLong64(&_buffer[_position + 8]);
            const uint64 v3 = BigEndian::readLong64(&_buffer[_position + 16]);
            const uint64 v4 = BigEndian::readLong64(&_buffer[_position + 24]);
            _position += 32;
            BigEndian::writeLong64(&bits[start + 0], (v0 << r) | (v1 >> a));
            BigEndian::writeLong64(&bits[start + 8], (v1 << r) | (v2 >> a));
            BigEndian::writeLong64(&bits[start + 16], (v2 << r) | (v3 >> a));
            BigEndian::writeLong64(&bits[start + 24], (v3 << r) | (v4 >> a));
            start += 32;
            remaining -= 256;
            _current = v4;
        }

        while (remaining >= 64) {
            const uint64 v = _current;
            _availBits = pullCurrent() - r;
            BigEndian::writeLong64(&bits[start], (v << r) | (_current >> _availBits));
            start += 8;
            remaining -= 64;
        }
    }

    // Last bytes
    while (remaining >= 8) {
        bits[start] = byte(readBits(8));
        start++;
        remaining -= 8;
    }

    if (remaining > 0)
        bits[start] = byte(readBits(remaining) << (8 - remaining));

    return count;
}

void DefaultInputBitStream::_close()
{
    if (isClosed() == true)
        return;

    _closed = true;

    // Reset fields to force a readFromInputStream() and trigger an exception
    // on readBit() or readBits()
    _read -= int64(_availBits); // can be negative
    _availBits = 0;
    _maxPosition = -1;
}

int DefaultInputBitStream::readFromInputStream(uint count)
{
    if (isClosed() == true)
        throw BitStreamException("Stream closed", BitStreamException::STREAM_CLOSED);

    if (count == 0)
        return 0;

    int size = -1;

    try {
        _read += (int64(_maxPosition + 1) << 3);
        _is.read(reinterpret_cast<char*>(_buffer), count);
        _position = 0;
        size = (_is.good() == true) ? int(count) : int(_is.gcount());
        _maxPosition = (size <= 0) ? -1 : size - 1;
    }
    catch (IOException& e) {
        throw BitStreamException(e.what(), BitStreamException::INPUT_OUTPUT);
    }

    if (size <= 0) {
       throw BitStreamException("No more data to read in the bitstream",
           BitStreamException::END_OF_STREAM);
    }

    return size;
}

// Return false when the bitstream is closed or the End-Of-Stream has been reached
bool DefaultInputBitStream::hasMoreToRead()
{
    if (isClosed() == true)
        return false;

    if ((_position < _maxPosition) || (_availBits > 0))
        return true;

    try {
        readFromInputStream(_bufferSize);
    }
    catch (BitStreamException&) {
        return false;
    }

    return true;
}

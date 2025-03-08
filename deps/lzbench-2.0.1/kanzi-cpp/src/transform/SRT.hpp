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

#pragma once
#ifndef _SRT_
#define _SRT_

#include "../Context.hpp"
#include "../Transform.hpp"

namespace kanzi {

   // Sorted Rank Transform is typically used after a BWT to reduce the variance
   // of the data prior to entropy coding.

   class SRT FINAL : public Transform<byte> {
   public:
       SRT() {}
       SRT(Context&) {}
       ~SRT() {}

       bool forward(SliceArray<byte>& pSrc, SliceArray<byte>& pDst, int length);

       bool inverse(SliceArray<byte>& pSrc, SliceArray<byte>& pDst, int length);

       int getMaxEncodedLength(int srcLen) const { return srcLen + MAX_HEADER_SIZE; }

   private:
       static const int MAX_HEADER_SIZE = 4 * 256;

       static int preprocess(const int freqs[], uint8 symbols[]);

       static int encodeHeader(int freqs[], byte dst[]);

       static int decodeHeader(const byte src[], int freqs[]);
   };
}
#endif


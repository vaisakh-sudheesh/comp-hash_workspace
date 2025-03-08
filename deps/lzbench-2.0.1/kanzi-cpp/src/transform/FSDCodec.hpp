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
#ifndef _FSDCodec_
#define _FSDCodec_

#include "../Context.hpp"
#include "../Transform.hpp"


// Fixed Step Delta codec
// Decorrelate values separated by a constant distance (step) and encode residuals
namespace kanzi {

   class FSDCodec FINAL : public Transform<byte> {

   public:
       FSDCodec() { _pCtx = nullptr; }

       FSDCodec(Context& ctx) : _pCtx(&ctx) {}

       ~FSDCodec() {}

       bool forward(SliceArray<byte>& src, SliceArray<byte>& dst, int length);

       bool inverse(SliceArray<byte>& src, SliceArray<byte>& dst, int length);

       // Required encoding output buffer size
       int getMaxEncodedLength(int srcLen) const
       {
           return srcLen + ((srcLen < 1024) ? 64 : srcLen >> 4); // limit expansion
       }

   private:
       static const int MIN_LENGTH = 1024;
       static const byte ESCAPE_TOKEN = byte(255);
       static const byte DELTA_CODING = byte(0);
       static const byte XOR_CODING = byte(1);
       static const uint8 ZIGZAG1[256];
       static const int8 ZIGZAG2[256];

       Context* _pCtx;
   };
}
#endif


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
#ifndef _AdaptiveProbMap_
#define _AdaptiveProbMap_

#include "../Global.hpp"

// APM maps a probability and a context into a new probability
// that the next bit will be 1. After each guess, it updates
// its state to improve future guesses.

namespace kanzi {
   template <int RATE>
   class LinearAdaptiveProbMap {
   public:
#if __cplusplus >= 202002L // simple-template-id in ctors and dtors rejected in C++20
       LinearAdaptiveProbMap(int n);

       ~LinearAdaptiveProbMap() { delete[] _data; }
#else
       LinearAdaptiveProbMap<RATE>(int n);

       ~LinearAdaptiveProbMap<RATE>() { delete[] _data; }
#endif

       int get(int bit, int pr, int ctx);

   private:
       int _index; // last p, context
       uint16* _data; // [NbCtx][33]:  p, context -> p
   };

   template <int RATE>
   inline LinearAdaptiveProbMap<RATE>::LinearAdaptiveProbMap(int n)
   {
       const int size = (n == 0) ? 65 : n * 65;
       _data = new uint16[size];
       _index = 0;

       for (int j = 0; j <= 64; j++) {
           _data[j] = uint16(j << 6) << 4;
       }

       for (int i = 1; i < n; i++) {
           memcpy(&_data[i * 65], &_data[0], 65 * sizeof(uint16));
       }
   }

   // Return improved prediction given current bit, prediction and context
   template <int RATE>
   inline int LinearAdaptiveProbMap<RATE>::get(int bit, int pr, int ctx)
   {
       // Update probability based on error and learning rate
       const int g = -bit & 65528;
       _data[_index] += (((g - int(_data[_index])) >> RATE) + bit);
       _data[_index + 1] += (((g - int(_data[_index + 1])) >> RATE) + bit);

       // Find index: 65*ctx + quantized prediction in [0..64]
       _index = (pr >> 6) + 65 * ctx;

       // Return interpolated probabibility
       const uint16 w = uint16(pr & 127);
       return int(_data[_index] * (128 - w) + _data[_index + 1] * w) >> 11;
   }



   template <bool FAST, int RATE>
   class LogisticAdaptiveProbMap {
   public:
#if __cplusplus >= 202002L // simple-template-id in ctors and dtors rejected in C++20
       LogisticAdaptiveProbMap(int n);

       ~LogisticAdaptiveProbMap() { delete[] _data; }
#else
       LogisticAdaptiveProbMap<FAST, RATE>(int n);

       ~LogisticAdaptiveProbMap<FAST, RATE>() { delete[] _data; }
#endif

       int get(int bit, int pr, int ctx);

   private:
       int _index; // last p, context
       uint16* _data; // [NbCtx][33]:  p, context -> p
   };

   template <bool FAST, int RATE>
   inline LogisticAdaptiveProbMap<FAST, RATE>::LogisticAdaptiveProbMap(int n)
   {
       const int mult = (FAST == false) ? 33 : 32;
       const int size = (n == 0) ? mult : n * mult;
       _data = new uint16[size];
       _index = 0;

       for (int j = 0; j < mult; j++)
           _data[j] = uint16(Global::squash((j - 16) * 128) << 4);

       for (int i = 1; i < n; i++)
           memcpy(&_data[i * mult], &_data[0], mult * sizeof(uint16));
   }

   // Return improved prediction given current bit, prediction and context
   template <bool FAST, int RATE>
   inline int LogisticAdaptiveProbMap<FAST, RATE>::get(int bit, int pr, int ctx)
   {
       // Update probability based on error and learning rate
       const int g = -bit & 65528;
       _data[_index] += (((g - int(_data[_index])) >> RATE) + bit);

       if (FAST == false) {
           _data[_index + 1] += (((g - int(_data[_index + 1])) >> RATE) + bit);
           pr = Global::stretch(pr);
           _index = ((pr + 2048) >> 7) + 33 * ctx;

           // Return interpolated probabibility
           const uint16 w = uint16(pr & 127);
           return int(_data[_index] * (128 - w) + _data[_index + 1] * w) >> 11;
       } else {
           _index = ((Global::stretch(pr) + 2048) >> 7) + 32 * ctx;
           return int(_data[_index]) >> 4;
       }
   }

}
#endif


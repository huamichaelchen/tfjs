/* Copyright 2019 Google Inc. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ===========================================================================*/

#include "src/cc/kernels.h"

#include <algorithm>

namespace tfjs {
namespace kernels {

// TODO(smilkov): Consider inlining small methods.

template <class T>
void add(T* a_buf, int a_size, T* b_buf, int b_size, T* out_buf) {
  int size = std::max(a_size, b_size);
  for (int i = 0; i < size; ++i) {
    out_buf[i] = a_buf[i % a_size] + b_buf[i % b_size];
  }
}

const int kBlockSize = 48;
void batchMatMul(float* a_buf, float* b_buf, int shared_dim, int left_dim,
                 int right_dim, int batch_dim, int a_batch, int a_outer_step,
                 int a_inner_step, int b_batch, int b_outer_step,
                 int b_inner_step, float* out_buf) {
  int size = left_dim * right_dim;

  // Zero out the output buffer because it might have been used before.
  std::fill(out_buf, out_buf + size, 0);

  for (int b = 0; b < batch_dim; b++) {
    for (int i0 = 0; i0 < left_dim; i0 += kBlockSize) {
      for (int j0 = 0; j0 < right_dim; j0 += kBlockSize) {
        for (int k0 = 0; k0 < shared_dim; k0 += kBlockSize) {
          // for when kBlockSize doesn't evenly divide the input
          int i_block = std::min(i0 + kBlockSize, left_dim);
          int j_block = std::min(j0 + kBlockSize, right_dim);
          int k_block = std::min(k0 + kBlockSize, shared_dim);

          for (int i = i0; i < i_block; i++) {
            for (int j = j0; j < j_block; j++) {
              float sum = 0.0;

              for (int k = k0; k < k_block; k++) {
                sum +=
                    a_buf[b * a_batch + i * a_outer_step + k * a_inner_step] *
                    b_buf[k * b_inner_step + j * b_outer_step + b * b_batch];
              }
              out_buf[b * size + (i * right_dim + j)] += sum;
            }
          }
        }
      }
    }
  }
}

// Templates need explicit instantiation when implemented in a .cc file.
template void add<float>(float* a_buf, int a_size, float* b_buf, int b_size,
                         float* out_buf);
template void add<int>(int* a_buf, int a_size, int* b_buf, int b_size,
                       int* out_buf);
template void add<bool>(bool* a_buf, int a_size, bool* b_buf, int b_size,
                        bool* out_buf);
}  // namespace kernels
}  // namespace tfjs

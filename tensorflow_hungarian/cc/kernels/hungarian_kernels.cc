/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if GOOGLE_CUDA
#define EIGEN_USE_GPU
#endif  // GOOGLE_CUDA

#include "hungarian.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/util/work_sharder.h"

using std::vector;
using shape_inference::Shape;
using shape_inference::Dimension;
using shape_inference::DimensionHandle;
using shape_inference::ShapeHandle;

namespace tensorflow {

typedef Eigen::ThreadPoolDevice CPUDevice;
typedef Eigen::GpuDevice GPUDevice;

namespace functor {

// CPU specialization of actual computation.
template <typename T>
struct HungarianFunctor<CPUDevice, T> {

    void operator()(const OpKernelContext* context,
                    const CPUDevice& d,
                    const int batch_size,
                    const int size_n,
                    const int size_m,
                    const T* costs,
                    T* assignments) {

        // implementation of the hungarian algorithm in c++
        auto shard_function = [&batch_size,
                               &size_n,
                               &size_m,
                               &costs,
                               &assignments](int64 start, int64 limit) {

            // batch-wise sharded function
            for (int batch = start; batch < limit; ++batch) {

                // pass

            }

        };

        // get a handle to the cpu device threads
        auto pool = context->device()->tensorflow_cpu_worker_threads();

        // computational cost of generating a single kernel operation
        const int64 op_cost = 10000 * size_n * size_n * size_m

        // launch a sharded batch function on the cpu
        Shard(pool->num_threads,
              pool->workers,
              batch_size,
              op_cost,
              shard_function);

    }

};

// OpKernel definition.
// template parameter <T> is the datatype of the tensors.
template <typename Device, typename T>
class HungarianOp : public OpKernel {

    public:

    explicit HungarianOp(OpKernelConstruction* context) : OpKernel(context) {}

    void Compute(OpKernelContext* context) override {

        // Grab the input tensor
        const Tensor& costs = context->input(0);

        // Create an output tensor that is null
        Tensor* assignments = NULL;

        // determine the shape of the output
        vector<int64> shape;
        for (int i = 0; i < costs.shape().dims(); ++i) {
            shape.push_back(costs.shape().dim_size(i));
        }

        // allocate space for the output tensor
        OP_REQUIRES_OK(context, context->allocate_output(
            0, TensorShape({shape[0], shape[1]}), &assignments));

        // check if the operation is too large
        OP_REQUIRES(context, costs.NumElements() <= tensorflow::kint32max,
                    errors::InvalidArgument("Too many elements in tensor"));

        // launch the device generalized operation functor
        HungarianFunctor<Device, T>()(
            context,
            context->eigen_device<Device>(),
            static_cast<int>(shape[0]),
            static_cast<int>(shape[1]),
            static_cast<int>(shape[2]),
            costs.tensor<T, 3>().data(),
            assignments->matrix<int>().data());

    }

};

// function that registers CPU operation kernels
#define REGISTER_CPU(T)                                            \
  REGISTER_KERNEL_BUILDER(                                         \
      Name("Hungarian").Device(DEVICE_CPU).TypeConstraint<T>("T"), \
      HungarianOp<CPUDevice, T>);

REGISTER_CPU(float);
REGISTER_CPU(int32);

// function that registers GPU operation kernels
#ifdef GOOGLE_CUDA
#define REGISTER_GPU(T)                                            \
  extern template struct HungarianFunctor<GPUDevice, T>;           \
  REGISTER_KERNEL_BUILDER(                                         \
      Name("Hungarian").Device(DEVICE_GPU).TypeConstraint<T>("T"), \
      HungarianOp<GPUDevice, T>);

REGISTER_GPU(float);
REGISTER_GPU(int32);
#endif  // GOOGLE_CUDA
}
}  // namespace tensorflow
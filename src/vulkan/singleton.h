// Copyright (c) 2018 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <type_traits>
#include <utility>

namespace Utils {
template <typename InstanceType> class Singleton {
  public:
    template <typename... ConstructorArgTypes>
    explicit Singleton(ConstructorArgTypes &&... constructor_args) {
        static_assert(
            sizeof(instance_storage_) >= sizeof(InstanceType),
            "instance_storage_ is not large enough to hold the instance");
        static_assert(alignof(decltype(instance_storage_)) >=
                          alignof(InstanceType),
                      "instance_storage_ does not meet the instance's "
                      "alignment requirement");
        new (&instance_storage_) InstanceType(
            std::forward<ConstructorArgTypes>(constructor_args)...);
    }
    ~Singleton() = default;

    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

    InstanceType *get() {
        return reinterpret_cast<InstanceType *>(&instance_storage_);
    }

  private:
    typename std::aligned_storage<
        sizeof(InstanceType), alignof(InstanceType)>::type instance_storage_;
};

} // namespace Utils

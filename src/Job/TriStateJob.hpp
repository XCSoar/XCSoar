// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Job.hpp"
#include "Operation/Operation.hpp"

/* damn you, windows.h! */
#ifdef ERROR
#undef ERROR
#endif

enum class TriStateJobResult {
  SUCCESS, ERROR, CANCELLED
};

/**
 * A wrapper that keeps track of whether the job was successful,
 * cancelled or whether it failed.
 */
template<typename T>
class TriStateJob final : public Job, public T {
private:
  TriStateJobResult result;

public:
  TriStateJob() = default;

  template<typename... Args>
  explicit TriStateJob(Args&&... args)
    :T(std::forward<Args>(args)...) {}

  TriStateJobResult GetResult() const {
    return result;
  }

  void Run(OperationEnvironment &env) override {
    result = T::Run(env)
      ? TriStateJobResult::SUCCESS
      : (env.IsCancelled()
         ? TriStateJobResult::CANCELLED
         : TriStateJobResult::ERROR);
  }
};

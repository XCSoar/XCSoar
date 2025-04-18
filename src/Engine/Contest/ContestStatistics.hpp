// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestResult.hpp"
#include "ContestTrace.hpp"

#include <array>
#include <type_traits>

struct ContestStatistics
{
  static constexpr std::size_t N = 4;

  std::array<ContestResult, N> result;
  std::array<ContestTraceVector, N> solution;

  void Reset() noexcept {
    for (auto &i : result)
      i.Reset();

    for (auto &i : solution)
      i.clear();
  }

  /**
   * Retrieve contest solution vector
   *
   * @param solution_index -1 for best, otherwise index of solution
   *
   * @return Vector of trace points selected for Contest
   */
  [[gnu::pure]]
  const ContestTraceVector &GetSolution(const int solution_index = -1) const noexcept {
    return solution[GetBestIndex(solution_index)];
  }

  [[gnu::pure]]
  const ContestResult &GetResult(const int solution_index = -1) const noexcept {
    return result[GetBestIndex(solution_index)];
  }

private:
  [[gnu::pure]]
  std::size_t GetBestIndex(const int solution_index) const noexcept {
    if (solution_index >= 0)
      return solution_index;

    return GetBestIndex();
  }

  [[gnu::pure]]
  std::size_t GetBestIndex() const noexcept {
    // Search for best solution by score
    double best = 0;
    std::size_t i_best = 0;
    for (std::size_t i = 0; i < result.size(); ++i) {
      if (result[i].IsDefined() && (result[i].score > best)) {
        // Better scored solution found
        i_best = i;
        best = result[i].score;
      }
    }

    // Return index to the best solution
    return i_best;
  }

};

static_assert(std::is_trivial_v<ContestStatistics>, "type is not trivial");

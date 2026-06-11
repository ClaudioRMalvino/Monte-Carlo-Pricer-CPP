#include "../include/europeanOption.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#ifdef _OPENMP
#include <omp.h>
#endif

EuropeanOption::EuropeanOption(double initStockPrice, double strikePrice,
                               double riskFreeIntRate, double volatility,
                               double timeToExpire, unsigned int seed)
    : m_S0{initStockPrice}, m_K{strikePrice}, m_r{riskFreeIntRate},
      m_sigma{volatility}, m_T{timeToExpire}, m_seed{seed}, m_generator{seed} {

  if (volatility < 0) {
    throw std::invalid_argument("volatility can only be a positive value");
  }
  if ((initStockPrice < 0) || (strikePrice < 0)) {
    throw std::invalid_argument(
        "initStockPrice and strikePrice can only be a positive value");
  }
  if (timeToExpire < 0) {
    throw std::invalid_argument("timeToExpire can only be a positive value");
  }
}

EuropeanOption::EuropeanOption(double initStockPrice, double strikePrice,
                               double riskFreeIntRate, double volatility,
                               double timeToExpire)
    : EuropeanOption(initStockPrice, strikePrice, riskFreeIntRate, volatility,
                     timeToExpire, std::random_device()()) {}

double EuropeanOption::_calculateST(double Z) const {
  // the analytic solution for Geometric Brownian motion (GBM)
  return m_S0 * std::exp((m_r - 0.5 * (m_sigma * m_sigma)) * m_T +
                         (m_sigma * std::sqrt(m_T) * Z));
}

std::pair<double, double>
EuropeanOption::calculatePrice(int numSimulations) const {
  if (numSimulations <= 0) {
    throw std::invalid_argument("numSimulations must be a positive value");
  }

  double payoffCall{0.0};
  double payoffPut{0.0};

  // Parallel Monte Carlo. Each thread draws from its own RNG stream, seeded
  // deterministically from the base seed plus its thread id. This keeps the
  // sampling thread-safe (no shared generator) and reproducible for a fixed
  // thread count, while the reduction accumulates the per-thread payoff sums
  // without data races.
#pragma omp parallel reduction(+ : payoffCall, payoffPut)
  {
#ifdef _OPENMP
    const unsigned int threadSeed =
        m_seed + static_cast<unsigned int>(omp_get_thread_num());
#else
    const unsigned int threadSeed = m_seed;
#endif
    std::mt19937 generator{threadSeed};
    std::normal_distribution<double> randGaussian(0.0, 1.0);

#pragma omp for schedule(static)
    for (int i = 0; i < numSimulations; i++) {
      const double Z = randGaussian(generator);
      const double pricePath = _calculateST(Z);

      // Accumulate the call and put payoffs for this path.
      payoffCall += std::max((pricePath - m_K), 0.0);
      payoffPut += std::max((m_K - pricePath), 0.0);
    }
  }

  // Average each payoff and discount it back to present value.
  const double avgPayoffCall = payoffCall / static_cast<double>(numSimulations);
  const double avgPayoffPut = payoffPut / static_cast<double>(numSimulations);
  const double discounted{std::exp(-m_r * m_T)};

  return std::make_pair(avgPayoffCall * discounted, avgPayoffPut * discounted);
}

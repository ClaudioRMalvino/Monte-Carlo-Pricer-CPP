#pragma once
#include <random>
#include <utility>

/**
 * @class EuropeanOption
 * @brief Prices a European Call/Put option using Monte Carlo.
 *
 * This class is an immutable, const-correct calculator.
 * Once constructed, its parameters cannot be changed.
 */
class EuropeanOption {
public:
  /**
   * @brief Construct a new European Option object with a random seed.
   *
   * @param initStockPrice The initial stock price (S0)
   * @param strikePrice The strike price (K)
   * @param riskFreeIntRate The risk-free interest rate (r)
   * @param volatility The volatility of the underlying (sigma)
   * @param timeToExpire The time to expiration in years (T)
   *
   * @throws std::invalid_argument if any parameters are negative.
   */
  EuropeanOption(double initStockPrice, double strikePrice,
                 double riskFreeIntRate, double volatility,
                 double timeToExpire);

  /**
   * @brief Construct a new European Option object with a fixed seed.
   *
   * This constructor is intended for testing and reproducibility,
   * as it guarantees the same simulation path for a given seed.
   *
   * @param initStockPrice The initial stock price (S0)
   * @param strikePrice The strike price (K)
   * @param riskFreeIntRate The risk-free interest rate (r)
   * @param volatility The volatility of the underlying (sigma)
   * @param timeToExpire The time to expiration in years (T)
   * @param seed The specific seed for the random number generator.
   *
   * @throws std::invalid_argument if any parameters are negative.
   */
  EuropeanOption(double initStockPrice, double strikePrice,
                 double riskFreeIntRate, double volatility, double timeToExpire,
                 unsigned int seed);
  /**
   * @brief Runs the Monte Carlo simulation to find the call/put prices.
   * @param numSimulations The number of simulation paths to run.
   * @return std::pair<double, double> (callPrice, putPrice)
   */
 [[nodiscard]] std::pair<double, double> calculatePrice(int numSimulations) const;

private:
  const double m_S0;                // Initial stock price
  const double m_K;                 // Strike price
  const double m_r;                 // Risk-free interest rate
  const double m_sigma;             // Volatility
  const double m_T;                 // Time to expire in years
  const unsigned int m_seed;          // Seed for rng
  mutable std::mt19937 m_generator; // Random number generator

  /**
   * @brief helper function calculates the price path through Geometric Brownian
   Motion (GBM)
   * when provided with a standard Gaussian random number, utilzing the
   following equation:
   *
   * S_T = S0 * std::exp( (r - 0.5 * sigma^2) * T + sigma*sqrt(T) * Z)
   *
   * where,
   *
   * S0: initial stock price
   * r:  risk-free interest rate
   * T: time to expire
   * sigma: volatility
   * Z: standard Gaussian random variable
   *
   *@param Z A single random variable from a std::normal_distribution(0, 1).

   * @return the price path after being translated by the propagator
   * */
  double _calculateST(double Z) const;
};

#include "../include/europeanOption.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#ifdef _OPENMP
    #include <omp.h>
#endif

namespace py = pybind11;

PYBIND11_MODULE(monte_carlo_pricer, m) {
  m.doc() =
      "A high-performance Monte Carlo option pricer utilizing C++ backend";

  py::class_<EuropeanOption>(m, "EuropeanOption")

      .def(py::init<double, double, double, double, double, unsigned int>(),
           "Create an option with a specific seed", py::arg("initStockPrice"),
           py::arg("strikePrice"), py::arg("riskFreeIntRate"),
           py::arg("volatility"), py::arg("timeToExpire"), py::arg("seed"))

      .def(py::init<double, double, double, double, double>(),
           "Create an option with a random seed", py::arg("initStockPrice"),
           py::arg("strikePrice"), py::arg("riskFreeIntRate"),
           py::arg("volatility"), py::arg("timeToExpire"))

      .def("calculatePrice", &EuropeanOption::calculatePrice,
           "Calculate the European Call and Put option prices",
           py::arg("numSimulations"), py::call_guard<py::gil_scoped_release>());
}

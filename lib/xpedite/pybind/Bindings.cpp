#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <xpedite/probes/Sample.H>
#include <xpedite/framework/SamplesLoader.H>
#include <sstream>
#include <iomanip>
#include <ios>

namespace py = pybind11;

using xpedite::probes::Sample;
using xpedite::framework::SamplesLoader;

PYBIND11_MODULE(xpediteBindings, m) {

  m.doc() = "Xpedite Samples Loader";

  py::class_<Sample>(m, "Sample")
    .def("size", &Sample::size)
    .def("returnSite", [](const Sample& sample) {
			return (uintptr_t) sample.returnSite();}
	  )
    .def("tsc", &Sample::tsc)
    .def("hasData", &Sample::hasData)
    .def("hasPmc", &Sample::hasPmc)
    .def("pmcCount", &Sample::pmcCount)
    .def("data", &Sample::data)
    .def("dataStr", [](const Sample& sample) {
        std::ostringstream stream;
        stream << std::hex << std::get<1>(sample.data()) << std::setw(16) << std::setfill('0') 
          << std::right << std::get<0>(sample.data()) << std::dec;
        return stream.str();
      }
    )
    .def("pmc", py::overload_cast<int>(&Sample::pmc, py::const_))
    .def("__repr__", &Sample::toString);

    py::class_<SamplesLoader>(m, "SamplesLoader")
        .def(py::init<const char*>())
        /// Bare bones interface
        .def(
            "__iter__",
            [](const SamplesLoader &s) { return py::make_iterator(s.begin(), s.end()); },
            py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */);
}

#include <xpedite/probes/Sample.H>
#include <xpedite/persistence/SamplesLoader.H>
#include <xpedite/txn/Counter.H>
#include <xpedite/txn/Txn.H>
#include <xpedite/txn/Route.H>
#include <xpedite/txn/TxnCollection.H>
#include <xpedite/txn/DataSource.H>
#include <xpedite/txn/TxnRepoLoader.H>
#include <xpedite/ux/UxProbe.H>

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <sstream>
#include <iomanip>
#include <ios>

namespace py = pybind11;

using xpedite::probes::Sample;
using xpedite::persistence::SamplesLoader;
using namespace xpedite::txn;
using namespace xpedite::ux;
using namespace xpedite::persistence;

PYBIND11_MAKE_OPAQUE(std::vector<SamplesLoader>);
PYBIND11_MAKE_OPAQUE(std::vector<Counter>);
PYBIND11_MAKE_OPAQUE(std::vector<const PackedString*>);
PYBIND11_MAKE_OPAQUE(Route::Probes);
PYBIND11_MAKE_OPAQUE(Events);
PYBIND11_MAKE_OPAQUE(TxnRepo::Benchmarks);
PYBIND11_MAKE_OPAQUE(Txns);
PYBIND11_MAKE_OPAQUE(ProbeHandles::Map);

PYBIND11_MODULE(xpediteBindings, m) {

  m.doc() = "Xpedite Txn Loader";

  py::bind_vector<Route::Probes>(m, "RouteProbes");
  py::bind_vector<Events>(m, "Events");
  py::bind_vector<std::vector<const PackedString*>>(m, "PackedStrings");
  py::bind_vector<std::vector<Counter>>(m, "Counters");
  py::bind_map<Txns>(m, "Txns");
  py::bind_map<TxnRepo::Benchmarks>(m, "TxnCollections");
  py::bind_map<ProbeHandles::Map>(m, "ProbeHandles");

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
        .def_static("saveAsCsv", &SamplesLoader::saveAsCsv)
        /// Bare bones interface
        .def(
            "__iter__",
            [](const SamplesLoader &s) { return py::make_iterator(s.begin(), s.end()); },
            py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */);

		py::class_<CpuInfo>(m, "CpuInfo")
      .def_property_readonly("cpuId", &CpuInfo::cpuId)
      .def_property_readonly("frequency", &CpuInfo::frequency)
      .def_property_readonly("frequencyKhz", &CpuInfo::frequencyKhz)
      .def_property_readonly("cyclesPerUsec", &CpuInfo::cyclesPerUsec)
      .def("convertCyclesToTime", &CpuInfo::convertCyclesToTime)
      .def("__repr__", &CpuInfo::toString);

		py::enum_<ProbeType>(m, "ProbeType")
			.value("Invalid", ProbeType::Invalid)
			.value("TxnBeginProbe", ProbeType::TxnBeginProbe)
			.value("TxnSuspendProbe", ProbeType::TxnSuspendProbe)
			.value("TxnResumeProbe", ProbeType::TxnResumeProbe)
			.value("TxnEndProbe", ProbeType::TxnEndProbe)
			.export_values();

    py::class_<UxProbe>(m, "UxProbe")
      .def(py::init<std::string, std::string, ProbeType>())
      .def("__repr__", &UxProbe::toString);

    py::class_<ProbeHandle>(m, "ProbeHandle")
      .def("uxProbe", &ProbeHandle::uxProbe)
      .def_property_readonly("name", &ProbeHandle::name)
      .def_property_readonly("sysName", &ProbeHandle::sysName)
      .def("getCanonicalName", &ProbeHandle::getCanonicalName)
      .def_property_readonly("isActive", &ProbeHandle::isActive)
      .def("isAnchored", &ProbeHandle::isAnchored)
      .def_property_readonly("isAnonymous", &ProbeHandle::isAnonymous)
      .def("probeName", &ProbeHandle::probeName, py::return_value_policy::automatic_reference)
      .def_property_readonly("fileName", &ProbeHandle::fileName, py::return_value_policy::automatic_reference)
      .def_property_readonly("functionName", &ProbeHandle::functionName, py::return_value_policy::automatic_reference)
      .def_property_readonly("lineNo", &ProbeHandle::lineNo)
      .def_property_readonly("canBeginTxn", &ProbeHandle::canBeginTxn)
      .def_property_readonly("canSuspendTxn", &ProbeHandle::canSuspendTxn)
      .def_property_readonly("canResumeTxn", &ProbeHandle::canResumeTxn)
      .def_property_readonly("canEndTxn", &ProbeHandle::canEndTxn)
      .def("__repr__", &ProbeHandle::toString);

    py::class_<ProbeHandles, ProbeHandlesPtr>(m, "Probes")
      .def("data", &ProbeHandles::data)
      .def("__repr__", &ProbeHandles::toString);

    py::class_<Route>(m, "Route")
      .def_property_readonly("probes", &Route::probes, py::return_value_policy::automatic_reference)
      .def("__len__", &Route::size)
      .def("__repr__", &Route::toString);

    py::class_<Counter>(m, "Counter")
      .def_property_readonly("threadId", &Counter::threadId)
      .def_property_readonly("tsc", &Counter::tsc)
      .def_property_readonly("pmcs", &Counter::pmcs)
      .def("hasData", &Counter::hasData)
      .def("hasPmc", &Counter::hasPmc)
      .def_property_readonly("data", &Counter::data)
      .def_property_readonly("probe", &Counter::probe)
      .def("__repr__", &Counter::toString);

		py::enum_<SampleFileFormat>(m, "SampleFileFormat")
      .value("Binary", SampleFileFormat::Binary)
      .value("CommaSeperatedValues", SampleFileFormat::CommaSeperatedValues)
      .export_values();

    py::class_<SampleFile>(m, "SampleFile")
      .def(py::init<uint64_t, uint64_t, std::string, SampleFileFormat>())
      .def("__repr__", &SampleFile::toString);

		py::enum_<DataSourceType>(m, "DataSourceType")
				.value("Current", DataSourceType::Current)
				.value("Benchmark", DataSourceType::Benchmark)
				.export_values();

    py::class_<DataSource>(m, "DataSource")
      .def(py::init<DataSourceType, std::string, std::vector<SampleFile>>())
      .def("__repr__", &DataSource::toString);

    py::class_<Txn, std::shared_ptr<Txn>>(m, "Txn")
      .def_property_readonly("txnId", &Txn::id)
      .def_property_readonly("counters", &Txn::counters)
      .def_property_readonly("route", &Txn::route, py::return_value_policy::reference)
      .def("hasProbe", &Txn::hasProbe)
      .def("getCounterForProbe", &Txn::getCounterForProbe, py::return_value_policy::reference)
      .def("getElapsedTsc", &Txn::getElapsedTsc, py::return_value_policy::reference)
      .def("__len__", &Txn::size)
      .def("__getitem__", [](const Txn& t_, size_t i_) {return t_[i_];}, py::return_value_policy::reference)
      .def("__repr__", &Txn::toString);

    py::class_<TxnCollection, std::shared_ptr<TxnCollection>>(m, "TxnCollection")
      .def("data", &TxnCollection::data, py::return_value_policy::reference)
      .def_property_readonly("name", &TxnCollection::name)
      .def_property_readonly("cpuInfo", &TxnCollection::cpuInfo)
      .def_property_readonly("probes", &TxnCollection::probes)
      .def_property_readonly("events",
          [](const TxnCollection& c_) {return *c_.events();},
          py::return_value_policy::reference
      )
      .def_property_readonly("topdownMetrics",
          [](const TxnCollection& c_) {return *c_.topdownNodes();},
          py::return_value_policy::reference
      )
      .def("getSubCollection", &TxnCollection::getSubCollection)
      .def("isCurrent", &TxnCollection::isCurrent)
      .def("__repr__", &TxnCollection::toString);

    py::class_<TxnSubCollection>(m, "TxnSubCollection")
      .def("data", &TxnSubCollection::data, py::return_value_policy::reference)
      .def("getSubCollection", &TxnSubCollection::getSubCollection)
      .def("append", &TxnSubCollection::append)
      .def_property_readonly("name", &TxnSubCollection::name)
      .def_property_readonly("cpuInfo", &TxnSubCollection::cpuInfo)
      .def_property_readonly("probes", &TxnSubCollection::probes)
      .def_property_readonly("events",
          [](const TxnSubCollection& c_) {return *c_.events();},
          py::return_value_policy::reference
      )
      .def_property_readonly("topdownMetrics",
          [](const TxnSubCollection& c_) {return *c_.topdownNodes();},
          py::return_value_policy::reference
      )
      .def("__len__", &TxnSubCollection::size)
      .def("__repr__", &TxnSubCollection::toString);

    py::class_<TxnRepo, std::shared_ptr<TxnRepo>>(m, "TxnRepo")
      .def("getCurrent", &TxnRepo::getCurrent)
      .def("getBenchmark", &TxnRepo::getBenchmark)
      .def("getBenchmarks", &TxnRepo::getBenchmarks)
      .def("hasBenchmarks", &TxnRepo::hasBenchmarks)
      .def("__repr__", &TxnRepo::toString);

    py::class_<TxnRepoLoader>(m, "TxnRepoLoader")
      .def(py::init<>())
      .def("data", &TxnRepoLoader::data)
      .def("load", &TxnRepoLoader::load)
      .def("__repr__", &TxnRepoLoader::toString);
}

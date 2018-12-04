"""
Classes to store xpedite profile data

  1. Profile - stores timeline statistics for a route taken by the application
  2. Profiles - a collection of profile objects, one for each distinct route

Author: Manikandan Dhamodharan, Morgan Stanley
"""

class Profile(object):
  """
  Stores timelnes and latency statistics for transactions with a common category/route
  """

  def __init__(self, name, current, benchmarks):
    self.name = name
    self.current = current
    self.benchmarks = benchmarks

  @property
  def category(self):
    """Category of transactions in this profile"""
    return self.current.category

  @property
  def route(self):
    """Route taken by transactions in this profile"""
    return self.current.route

  @property
  def probes(self):
    """Probes enabled for this profile"""
    return self.current.probes

  @property
  def reportProbes(self):
    """Human friendly names for probes in this profile"""
    return self.current.reportProbes

  @property
  def pmcNames(self):
    """Names of pmu event collected in this profile"""
    return self.current.pmcNames

  @property
  def topdownKeys(self):
    """Topdown nodes computed by this profile"""
    return self.current.topdownKeys

  @property
  def events(self):
    """Pmu events collected in this profile"""
    return self.current.events

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

  def __repr__(self):
    profileStr = 'profile for {} \n\t route {}'.format(self.category, self.route)
    profileStr += '\n\t {} - timelines {}'.format(self.current.name, len(self.current))
    for benchmark in self.benchmarks.values():
      profileStr += '\n\t benchmark {} - timelines {}'.format(benchmark.name, len(self.current))
    return profileStr

class Profiles(object):
  """
  Collection of profiles

  Profiles stores a collection of Profile objects, one for each category and route combination
  """

  def __init__(self, name, transactionRepo):
    self.name = name
    self.transactionRepo = transactionRepo
    self.profiles = []

  def addProfile(self, profile):
    """
    Adds profile to this collection

    :param profile: Profile to be added

    """
    self.profiles.append(profile)

  def makeBenchmark(self, path):
    """
    Persists samples for current run in the given path for future benchmarking

    :param path: Path to persist profiles for the current session

    """
    from xpedite import benchmark
    return benchmark.makeBenchmark(self, path)

  @property
  def cpuInfo(self):
    """Cpu Info of the host running the current profile session"""
    return self.profiles[0].current.cpuInfo if self.profiles else None

  @property
  def pmcNames(self):
    """A list of pmu event names collected in the current profile session"""
    return self.profiles[0].pmcNames if self.profiles else None

  @property
  def topdownKeys(self):
    """A list of topdown nodes computed in the current profile session"""
    return self.profiles[0].topdownKeys if self.profiles else None

  @property
  def events(self):
    """A list of pmu events collected in the current profile session"""
    return self.profiles[0].events if self.profiles else None

  @property
  def eventsMap(self):
    """Returns a map of event names to index for computation of topdown metrics"""
    return self.profiles[0].current.deltaSeriesRepo.buildEventsMap() if self.profiles else None

  def __len__(self):
    return len(self.profiles)

  def __getitem__(self, index):
    return self.profiles[index]

  def __repr__(self):
    profilesStr = ''
    for profile in self.profiles:
      profilesStr += '\n{}'.format(profile)
    return profilesStr

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

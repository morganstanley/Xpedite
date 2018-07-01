"""
Route object

A Route is a ordered sequence of probes, that got hit in a transaction.
The ordering of probes in a route matches the flow of control (program order).
Routes are used for aggregating and conflating transaction objects.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

class Route(object):
  """A sequence of probes in program execution order"""

  def __init__(self, *probes):
    self.probes = tuple(*probes)
    self.points = tuple(probe.sysName for probe in self.probes)

  def __len__(self):
    return len(self.points)

  def __hash__(self):
    return hash(self.points)

  def __eq__(self, other):
    return self.points == self.points

  def __repr__(self):
    return 'Route: ' + str(self.points)

def conflateRoutes(srcRoute, dstRoute):
  """
  Conflates the given source route to destination route

  For two given routes, this method combines probes (conflates),
    if srcRoute route is a super set of the dstRoute

  :param srcRoute: Route to be conflated to destination route
  :param dstRoute: Traget route to conflate to

  """
  indices = []
  probeIndex = len(dstRoute) - 1
  for i, point in enumerate(reversed(srcRoute.points)):
    if probeIndex < 0:
      break
    if dstRoute.points[probeIndex] == point:
      indices.append(len(srcRoute) - (i + 1))
      probeIndex -= 1
  if len(dstRoute) == len(indices):
    return indices[::-1]
  return None

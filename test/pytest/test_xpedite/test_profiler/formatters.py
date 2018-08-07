"""
Create class specific messages to use when comparing objects
with the Xpedite comparator

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

class Formatters(object):
  """Format strings to be logged by the comparator"""
  formatters = {
    'Transaction':   lambda txn: ' Transaction ID: {}'.format(txn.txnId),
    'Timeline':      lambda timeline: ' Timeline Transaction ID: {}'.format(timeline.txnId),
    'TimelineStats': lambda tls: ' Timeline Statistics Name: {}'.format(tls.name),
    'AnchoredProbe': lambda probe: ' Probe Name: {}'.format(probe.sysName),
    'TopdownValue':  lambda topdown: ' Topdown - Name: {}, Value: {}'.format(topdown.name, topdown.value),
    'TimePoint':     lambda point: ' Time Point Name: {}'.format(point.name),
    'Counter':       lambda counter: ' Counter Probe: {}'.format(counter.probe.sysName),
    'Event':         lambda event: ' Event Name: {}'.format(event.name),
    'DeltaSeries':   lambda series: ' Delta Series: {} -> {}'.format(series.beginProbeName, series.endProbeName),
    'Profile':       lambda profile: ' Profile Name: {}'.format(profile.name),
  }

"""
Module to generate histogram for plotting latency distributions.

This module provides logic to build buckets and distributions for a
collection of values. The bucket values and counts are plotted in x and y
axis respectively.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import bisect

class Histogram(object):
  """Stores data and attributes needed for creating histograms"""

  def __init__(self, title, description, data, options, statistics=None, width=1000, height=500, checked=None):
    """
    Constructs a latency distribution visaulization histogram

    :param name: Category of the histogram
    :param description: Description of the histogram
    :param width: Width of the histogram (Default value = 1000)
    :param height: Height of the histogram (Default value = 500)

    """

    self.title = title
    self.description = description
    self.data = data
    self.options = options
    self.statistics = statistics
    self.width = width
    self.height = height
    self.checked = checked

def formatLegend(prefix, minimum, maximum, mean, median, percentile95, percentile99):
  """
  Formats legend with statistics data for a histogram

  :param prefix: Prefix for the label
  :param minimum: Min value in the plotted series
  :param maximum: Max value in the plotted series
  :param mean: Mean value of the plotted series
  :param percentile95: 95 percentile value of the plotted series
  :param percentile99: 99 percentile value of the plotted series
  :returns: The formatted label as string

  """
  legend = (
    '{} (min. = {:0.2f}us, max. = {:0.2f}us, mean = {:0.2f}us, median = {:0.2f}us, '
    '95% = {:0.2f}us, 99% = {:0.2f}us)'
  ).format(prefix, minimum, maximum, mean, median, percentile95, percentile99)
  return legend

def buildHistograms(ticks, series, stack=False):
  """
  Builds options and data series for histograms

  :param ticks: The values to plot on the x axis
  :param series: The collection of data points to build distribution from
  :param stack: Controls whether to stack the bars or offset them (Default value = False)
  :returns: the histogram options and the data series ready to be plotted

  """
  totalWidth = 0.8
  options = {'xaxis': {'ticks': list(zip(range(0, len(ticks)), ticks))}}

  if stack:
    options['series'] = {'stack': True}

  data = []
  width = totalWidth / (((len(series) + 1)/2)*2)
  index = None

  def offset(xval):
    """Calculates horizontal offset"""
    if stack:
      return xval
    return xval - totalWidth / 2 + (index + 1) * width

  for index, _ in enumerate(series):
    name, serie = series[index]
    if len(serie) > len(ticks):
      raise ValueError('Series at index {} has more elements than there are ticks'.format(index))

    data.append(
      {
        'label': name,
        'data': list(zip([offset(centre) for centre in range(0, len(serie))], serie)),
        'shadowSize': 4,
        'bars':
        {
          'show': True,
          'barWidth': totalWidth if stack else width * 0.8,
          'fillColor':
          {
            'colors':[{'opacity': 0.8}, {'brightness': 0.6, 'opacity': 0.8}]
          }
        }
      }
    )
  return options, data

def buildBuckets(series, bucketCount):
  """
  Builds buckets for given elapsed tsc distribution bundle

  :param series: series of values
  :param bucketCount: number of buckets

  """
  series = sorted(series)
  confidence = series[0:int(.95 * len(series))]
  total = sum(confidence)
  count = len(confidence)
  if count <= 0:
    return None
  mean = total / count
  lowerBound = mean / 2
  upperBound = mean * 2
  bucketSize = (upperBound - lowerBound) / bucketCount
  buckets = []
  if bucketSize != 0:
    bucket = lowerBound
    while bucket <= upperBound:
      buckets.append(bucket)
      bucket += bucketSize
  return buckets

def buildDistribution(buckets, valueSeries):
  """
  Builds distribution for the given value series

  :param buckets: buckets in the histogram
  :param valueSeries: series to build distribution from

  """
  bucketValues = [0] * len(buckets)
  conflatedCountersCount = 0
  for value in valueSeries:
    index = bisect.bisect_left(buckets, value)
    if index < len(bucketValues):
      bucketValues[index] += 1
    else:
      bucketValues[len(bucketValues) - 1] += 1
      conflatedCountersCount += 1
  return bucketValues, conflatedCountersCount

def formatBuckets(buckets):
  """
  Formats buckets into a list of strings

  :param buckets: buckets to be formatted

  """
  timeList = []
  for i, bucket in enumerate(buckets):
    formatStr = '{0:4,.3g}' if i < len(buckets) -1 else '>>{0:4,.3g}'
    timeList.append(formatStr.format(bucket))
  return timeList

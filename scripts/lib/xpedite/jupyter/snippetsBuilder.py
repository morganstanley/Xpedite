"""
Module to generate snippets from routes in profile objects

This module builds a json representation of commnly used snippets
for integration with Jupyter sinppets plugin

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

import json

def breakCommand(cmd, lineSize=150):
  """Formats commands with line breaks"""
  fmtCmd = ''
  begin = 0
  end = min(lineSize+1, len(cmd))
  while end - begin > lineSize:
    index = cmd.rfind(',', begin, end)
    if index == -1:
      fmtCmd += cmd[begin:end]
      begin = end
    else:
      fmtCmd += cmd[begin:index+1] + '\n'
      begin = index+1
    end = min(begin + lineSize+1, len(cmd))
  fmtCmd += cmd[begin:end]
  return fmtCmd

def buildSnippets(profiles):
  """Builds snippets for the current profile session"""
  snippets = []
  for count, profile in enumerate(profiles):
    route = profile.current.route.points
    routeProbes = '({})'.format(list(route))
    routeName = ' [{}] | {}->{}'.format(len(route), route[0], route[-1])
    txnCount = len(profile.current.timelineCollection)
    diffArgs = '({}, {})'.format(
      profile.current.timelineCollection[0].txnId, profile.current.timelineCollection[txnCount - 1].txnId
    )
    txnsDiffArgs = '([{}, {}], [{}, {}])'.format(
      profile.current.timelineCollection[0].txnId, profile.current.timelineCollection[int(txnCount / 4)].txnId,
      profile.current.timelineCollection[int(txnCount / 2)].txnId,
      profile.current.timelineCollection[txnCount - 1].txnId
    )

    txnCode = [breakCommand('txns{}'.format(routeProbes))]
    plotTxnCode = [breakCommand('plot{}'.format(routeProbes))]
    filterCode = [breakCommand('filter(lambda txn: txn.duration > 0).txns{}'.format(routeProbes))]
    filterStatCode = [breakCommand('filter(lambda txn: txn.duration > 0).stat{}'.format(routeProbes))]
    diffCode = [breakCommand('diff{}'.format(diffArgs))]
    txnsDiffCode = [breakCommand('diff{}'.format(txnsDiffArgs))]

    snippets.append({'name':'txns{}'.format(routeName), 'code':txnCode})
    snippets.append({'name':'plot{}'.format(routeName), 'code':plotTxnCode})
    snippets.append({'name':'filter{}'.format(routeName), 'code':filterCode})
    snippets.append({'name':'filterStat{}'.format(routeName), 'code':filterStatCode})
    snippets.append({'name':'txnDiff{}'.format(routeName), 'code':diffCode})
    if len(profile.current) > 3:
      snippets.append({'name':'txnsDiff{}'.format(routeName), 'code':txnsDiffCode})

    if count >= 5:
      break
  snippets.append({'name':'plot-one', 'code':['plot()']})
  return json.dumps(snippets)

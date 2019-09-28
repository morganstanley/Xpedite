"""
Module defines the backend for live chart widget.

Author: Dhruv Shekhawat, Morgan Stanley.

"""
import asyncio
import ipywidgets as widgets
from traitlets import Unicode, List, Dict
from xpedite.jupyter.widgets import waitForChange

class LiveChartWidget(widgets.DOMWidget):
  """Class to associate front end with back end."""
  _view_name = Unicode('LiveChartView').tag(sync=True)
  _view_module = Unicode('live-chart').tag(sync=True)
  _view_module_version = Unicode('0.1.0').tag(sync=True)
  txnTicks = List([]).tag(sync=True)
  txnBrush = Dict({}).tag(sync=True)

if 'liveWidget' not in globals():
  liveWidget = LiveChartWidget()

def createAsyncCallbacks():
  """Create callback for brush slider."""
  @asyncio.coroutine
  def waitForTxnBrush():
    while True:
        data = yield from waitForChange(liveWidget, 'txnBrush')
  asyncio.ensure_future(waitForTxnBrush())

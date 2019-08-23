"""
Module defines the backend for histogram widget.

Author: Dhruv Shekhawat, Morgan Stanley.

"""
import asyncio
import ipywidgets as widgets
from traitlets import Unicode, List, Dict
from xpedite.jupyter.widgets import wait_for_change

class HistogramWidget(widgets.DOMWidget):
    _view_name = Unicode('HistogramView').tag(sync=True)
    _view_module = Unicode('histogram').tag(sync=True)
    _view_module_version = Unicode('0.1.0').tag(sync=True)
    evts = List([]).tag(sync=True)
    txn_drilldown = List([]).tag(sync=True)
    evt_drilldown = Dict({}).tag(sync=True)
    click = Dict({}).tag(sync=True)

if 'flotwidget' not in globals():
    flotwidget = HistogramWidget()

def create_async_callbacks():
    @asyncio.coroutine
    def wait_for_click():
        while True:
            data = yield from wait_for_change(flotwidget, 'click')
    asyncio.ensure_future(wait_for_click())

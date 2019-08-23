"""
Module defines the backend for ECG widget.

Author: Dhruv Shekhawat, Morgan Stanley.

"""
import asyncio
import ipywidgets as widgets
from traitlets import Unicode, List, Dict, Int
from xpedite.jupyter.widgets import wait_for_change

class EcgWidget(widgets.DOMWidget):
    _view_name = Unicode('EcgView').tag(sync=True)
    _view_module = Unicode('ecg').tag(sync=True)
    _view_module_version = Unicode('0.1.0').tag(sync=True)
    evt_ticks = List([]).tag(sync=True)
    evt_brush = Dict({}).tag(sync=True)
    current_core = Int().tag(sync=True)

if 'ecgwidget' not in globals():
    ecgwidget = EcgWidget()

def create_async_callbacks():
    @asyncio.coroutine
    def wait_for_evt_brush():
        while True:
            data = yield from wait_for_change(ecgwidget, 'evt_brush')
    asyncio.ensure_future(wait_for_evt_brush())

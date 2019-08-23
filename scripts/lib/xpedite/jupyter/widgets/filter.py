"""
Module defines the backend for Filter widget.

Author: Dhruv Shekhawat, Morgan Stanley.

"""
import asyncio
import ipywidgets as widgets
from traitlets import Unicode, Dict, Int
from xpedite.jupyter.widgets import wait_for_change

class FilterWidget(widgets.DOMWidget):
    _view_name = Unicode('FilterView').tag(sync=True)
    _view_module = Unicode('filter').tag(sync=True)
    _view_module_version = Unicode('0.1.0').tag(sync=True)
    tab = Unicode().tag(sync=True)

if 'filterwidget' not in globals():
    filterwidget = FilterWidget()

def create_async_callbacks():
    @asyncio.coroutine
    def wait_for_tab_change_filter():
        while True:
            data = yield from wait_for_change(filterwidget, 'tab')
    asyncio.ensure_future(wait_for_tab_change_filter())

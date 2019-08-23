"""
Module defines callback which triggers when a widget attribute changes.

Author: Dhruv Shekhawat, Morgan Stanley.

"""
import asyncio

def wait_for_change(widget, value):
  """ Wait for change """
  future = asyncio.Future()
  def getvalue(change):
    """ Get value """
    future.set_result(change.new)
    widget.unobserve(getvalue, value)
    widget.observe(getvalue, value)
    return future

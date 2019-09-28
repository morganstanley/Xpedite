"""
Module to asynchronously track any changes in a widget's attributes.

Author: Dhruv Shekhawat, Morgan Stanley.
"""
import asyncio # pylint: disable=import-error

def waitForChange(widget, value):
  """Track value change of widget."""
  future = asyncio.Future()
  def getvalue(change):
    """get attr value."""
    future.set_result(change.new)
    widget.unobserve(getvalue, value)
  widget.observe(getvalue, value)
  return future

"""
This module generates markup for syntax highlighting of code
in html reports

Author:  Brooke Elizabeth Cantwell, Morgan Stanley
"""

from pygments import highlight, lexers, formatters
import inspect

class CodeFormatter(object):
  """Formatter to render code with syntax highlighting"""

  @staticmethod
  def format(code, _, wrapperBegin, wrapperEnd):
    """Generates markup for highlighting syntax in the given code"""
    if inspect.isfunction(code):
      sourceCode = inspect.getsource(code)
    else:
      sourceCode = inspect.getsource(type(code))
    formatter = formatters.HtmlFormatter(style='monokai')
    formattedCode = highlight(sourceCode, lexers.PythonLexer(), formatter)
    formattedCode = wrapperBegin + formattedCode + wrapperEnd
    return formattedCode

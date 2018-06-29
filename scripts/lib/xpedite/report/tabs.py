"""
Module to support rendering html tabs

Author: Manikandan Dhamodharan, Morgan Stanley
"""

TAB_JS = """
<script>
$(document).ready(function () {
  $(".xpedite-tabs-panel a").click(function(){
    $(this).tab('show');
  });
});
</script>
"""
TAB_HEADER_FMT = '<li class="{1}"><a href="#{0}">{2}</a></li>'

TAB_CONTAINER_FMT = """
<div>
  <ul class="xpedite-tabs-panel">
  {}
  </ul>
  {}
</div>
"""

TAB_BODY_PREFIX = '<div class="xpedite-tab-content">'

TAB_BODY_SUFFIX = '</div>'

TAB_BODY_FMT = """
  <div id="{}" class="xpedite-tab-pane {}">
    {}
  </div>
"""

def tabState(state):
  """Returns css selector based on tab state"""
  return 'active' if state else ''

def tabContentState(state):
  """Returns css selector based on tab content state"""
  return 'active' if state else 'fade'

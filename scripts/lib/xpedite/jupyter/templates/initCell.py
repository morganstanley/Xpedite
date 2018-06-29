"""Xpedite Prologue"""
INTRO_FRMT = """<h2> Welcome to {appName} performance analytics shell !</h2>
<p class="xpediteIntro">
 This shell empowers power users to do, advanced transaction level latency analysis and drill down.
 The cells below provide {appName}'s latency distribution histograms, for {categoryCount} different categories of transactions.
 Each histogram shows the number of transactions (y-axis), that fall into a specific latency envelope (x-axis).
 The detailed performance statistics are accessible using links below the histogram.
 For transactions, that take multiple routes (control flow in code), a report will be generated for each such route.
</p>
<p>
  <ul>
    <li> To see more details about the environment, where the profile was generated, click <a href='{envLink}' target='_blank'>Test Env</a>.</li>
    <li> Interested in knowing more about this shell? Checkout more detailed docs at <a href='http://xpedite' target='_blank'>xpedite</a>.</li>
    <li> To get help from a friendly human, email <a href="mailto:msperf@morganstanley.com?subject=Xpedite shell support">msperf</a>.</li>
    <li> To regenerate the report run - "xpedite report -p profileInfo.py -r {runId}"</li>
  </ul>
</p>
"""

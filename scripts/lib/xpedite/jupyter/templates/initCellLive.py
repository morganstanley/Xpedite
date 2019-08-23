"""Xpedite Prologue"""
INTRO_FRMT = """<h2> Welcome to {appName} performance analytics shell !</h2>
<p class="xpediteIntro">
 This shell empowers power users to do, advanced transaction level latency analysis and drill down.
 The cells below provide {appName}'s latency distribution histograms with realtime update.
 Each histogram shows the number of transactions (y-axis), that fall into a specific latency envelope (x-axis).
 The detailed performance statistics are accessible using links below the histogram.
 For transactions, that take multiple routes (control flow in code), a report will be generated for each such route.
</p>
<p>
  <ul>
    <li> Interested in knowing more about this shell? Checkout more detailed docs at <a href='http://xpedite' target='_blank'>xpedite</a>.</li>
    <li> To get help from a friendly human, email <a href="mailto:msperf@morganstanley.com?subject=Xpedite shell support">msperf</a>.</li>
  </ul>
</p>

<div class="tab">
    <button class="tablink layoutText" onclick="openPage('orders')" id="ecg-tab-btn">ECG</button>
    <button class="tablink layoutText" onclick="openPage('probes')">Probes</button>
</div>

<div id="orders" class="tabcontent">
    <div id="evtViewDiv"></div>
    <label id="evtecgswitch" class="switch"> 
        <input id="evthalt" type="checkbox"> 
        <span class="slider round"></span> 
    </label>
    <p id="evtecgfreeze" class="layoutText">Halt</p>

</div>
<div id="probes" class="tabcontent">
</div>
<script src="/static/layout.js"></script>
<script src="/static/ecgwidget.js"></script>
<script>instantiateWidget();</script>
"""
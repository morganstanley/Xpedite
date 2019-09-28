function instantiateWidget() {
    require.undef('live-chart');

    define('live-chart', ['@jupyter-widgets/base'], function(widgets) {
        var liveChart;
        var LiveChartView = widgets.DOMWidgetView.extend({
            render: function() {
                var widget = this; //temp variable needed.
                this.model.on('change:txn_ticks', this.txnTicksChanged, this);
                require(['d3', 'livechart'], function(d3, livechart) {
                    liveChart = createLiveChart(widget, '#liveChart', '#evthalt');
                });
            },

            ticksChanged: function(widget, ticks, chart) {
                var newData = widget.model.get(ticks);
                require(['d3', 'livechart'], function(d3, livechart) {
                    for(var i = 0; i < newData.length; i++) {
                        var latency = newData[i]['latency'];
                        var epoch = newData[i]['position'];
                        var position = new Date(epoch);
                        var spike = {
                            value: latency,
                            time: position,
                            color: newData[i]['color'],
                            ts: epoch,
                        };
                        chart.datum(spike);
                    }
                });
            },

            txnTicksChanged: function() {
                var callback = this.tickChanged;
                callback(this, 'txn_ticks', liveChart);
            },
        });

        return {
            LiveChartView : LiveChartView
        };
    });
}

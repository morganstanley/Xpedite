function createParentSVG(selection, height, width) {
    var chartSvg = selection
        .append("svg")
        .style("border", "1px solid lightgray")
        .attr("height", height)
        .attr("width", width);
    return chartSvg;
}

function createBoard(svg, leftMargin, topMargin) {
    var tickerBoard = svg
        .append("g")
        .attr("transform", "translate (" + leftMargin + "," + topMargin + ")");
    return tickerBoard;
}

function buildTickerBoard(board, x, y, title, clipID, height, width) {
    buildText(board, "chartTitle", "rotate(0)", x, y, title);
    board.append("clipPath")
        .attr("id", clipID)
      .append("rect")
        .attr("width", width)
        .attr("height", height)
        .style("fill", "#3a3a3a");
}

function buildBrushBoard(board, width, height) {
    board.append("rect")
        .attr("width", width)
        .attr("height", height)
        .style("fill", "#3a3a3a")
        .style("shape-rendering", "crispEdges");
}


function createAxis(board, className, translateAxis, translateText, x, y, title) {
    var axis = board.append("g")
        .attr("class", className)
        .attr("transform", translateAxis);
    buildText(axis, "title", translateText, x, y, title);
    return axis;
}

function buildText(element, className, translateText, x, y, title) {
    element.append("text")
        .attr("class", className)
        .attr("transform", translateText)
        .attr("x", x)
        .attr("y", y)
        .attr("dy", ".70em")
        .text(title);
    return element;
}

function liveChart(widget, brushCallback) {

  var clipID = "chartClip",
      intervalTime = 200, height = 240, heightBrush = 40, width = 620,
      pastDuration = 300, areaPerSecond = 10,
      startTime = 0, endTime = 0,
      tickWidth = 1,
      halted = false,
      chartTitle = "Live Txns - 5 min", yTitle = "Latency (μs)", xTitle = "Time",
      tickId = 0;

  var chart = function(selection) { 
    var xAxis = d3.svg
        .axis()
        .orient("bottom");
    var xScale = d3.time
        .scale()
        .range([0, width]);
    var yAxis = d3.svg
        .axis()
        .orient("left");
    var yScale = d3.scale
        .linear()
        .domain([0, 25])
        .range([height, 0]);

    var chartSvg = createParentSVG(selection, 400, 700);
    var tickerBoard = createBoard(chartSvg, 50, 40);
    buildTickerBoard(tickerBoard, width/2, -20, chartTitle, clipID, height, width);
    var brushBoard = createBoard(chartSvg, 50, 330);
    buildBrushBoard(brushBoard, width, heightBrush);
    var tickingArea = tickerBoard.append("g").attr("clip-path", "url(#" + clipID);
    var brushingArea = brushBoard.append("g").attr("class", "brush");
    var tickerXAxis = createAxis(tickerBoard, "x axis", "translate(0," + height + ")", "rotate(0)", width/2, 25, xTitle);
    var tickerYAxis = createAxis(tickerBoard, "y axis", "rotate(0)", "rotate(-90)", -height/2, -35, yTitle);

    var xAxisBrushG = brushBoard.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + heightBrush + ")");

    var xBrush = d3.time.scale().range([0, width]);
    var yBrush = d3.scale.linear().domain([0, 25]).range([heightBrush, 0]);

    var xAxisBrush = d3.svg.axis().orient("bottom");

    function initBrushExtent(startTime, endTime) {
      var brushEnd = endTime;
      var brushStart = new Date(endTime.getTime() - width / areaPerSecond * 1000);
      return [brushStart, brushEnd];
    }

    function updateBrushExtent(interval, offset) {
      return(startTime, endTime) => {
        var brushStart = new Date(startTime.getTime() + offset);
        var brushEnd = new Date(brushStart.getTime() + interval);
        return [brushStart, brushEnd];
      }
    }

    function updateXAxes(callback) {
      endTime = new Date();
      startTime = new Date(endTime.getTime() - pastDuration * 1000);
      xBrush.domain([startTime, endTime]); 
      var brushExtent = callback(startTime, endTime);
      xScale.domain(brushExtent);
      xAxis.scale(xScale)(tickerXAxis);
      xAxisBrush.scale(xBrush)(xAxisBrushG);
      return brushExtent;
    }
    
    var brushExtent = updateXAxes(initBrushExtent);
    yAxis.scale(yScale)(tickerYAxis);

    var filterPane = d3.svg.brush()
        .x(xBrush)
        .extent(brushExtent)
        .on("brushend", function(){
          if(brushCallback)
              brushCallback();
        })
        .on("brush", function () {
          xScale.domain(filterPane.empty() ? xBrush.domain() : filterPane.extent());
          xAxis.scale(xScale)(tickerXAxis);
          refresh();
        });

    var filterPaneG = brushBoard.append("g")
        .attr("class", "filterPane")
        .call(filterPane)
        .selectAll("rect")
        .attr("height", heightBrush);

    var data = [];
    refresh();

    function refreshArea(area, xScale, yScale, height) {
      var allTicks = area.selectAll(".tick")
          .data(data);
      allTicks.exit().remove();
      allTicks.enter().append("rect")
          .attr("class", "tick")
          .attr("id", function() { 
            return "tick-" + tickId++; 
          })
          .attr("shape-rendering", "crispEdges");
      allTicks
          .attr("width", tickWidth)
          .attr("height", function(d) { return height - yScale(d.value); })
          .attr("x", function(d) { return Math.round(xScale(d.time) - tickWidth); })
          .attr("y", function(d) { return yScale(d.value); })
          .style("fill", function(d) { return d.color; })
          .style("fill-opacity", 1);
    }

    function refresh() {
      data = data.filter(function(d) {
        if (d.time.getTime() > startTime.getTime() &&
            d.time.getTime() < endTime.getTime()) 
          return true;
      })
      refreshArea(tickingArea, xScale, yScale, height);
      refreshArea(brushingArea, xBrush, yBrush, heightBrush);
    }

  chart.push = function(val) {
    data.push(val);
  }

  chart.halt = function(val) {
    halted = val;
  }
    setInterval(function() {
      if (halted) return;
      var extent = filterPane.empty() ? xBrush.domain() : filterPane.extent();
      var interval = extent[1].getTime() - extent[0].getTime();
      var offset = extent[0].getTime() - xBrush.domain()[0].getTime();
      var callback = updateBrushExtent(interval, offset);
      var brushExtent = updateXAxes(callback);
      filterPane.extent(brushExtent);
      refresh();
    }, intervalTime)
    return chart;
  }

  return chart;
}

function createLiveChart(widget, domID, haltID){
    var chart = liveChart(widget);

    var chartDiv = d3.select(domID).append("div")
        .attr("id", domID + "chartDiv")
        .call(chart);

    d3.select(haltID).on("change", function() {
      var state = d3.select(haltID).property("checked");
      chart.halt(state);
    })

    return chart;
}

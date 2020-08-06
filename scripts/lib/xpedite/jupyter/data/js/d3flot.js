function getInitCell(){
    var cells = Jupyter.notebook.get_cells();
    for(cell in cells){
        if(cells[cell]["metadata"]["isInit"] == '0xFFFFFFFFA5A55A5DUL'){
            return cells[cell];
        }
    }
}

function createFlot(cellNum){
    var cell = getInitCell();
    var d3Flot = cell["metadata"]["d3Flots"][cellNum];
    var xyCoords = d3Flot.xyCoords;
    var xAxisValues = d3Flot.xAxisValues;
    var xAxisLabels = d3Flot.xAxisLabels;
    var legends = d3Flot.legends;

    //colors for legend and bars
    var color = d3.scale.ordinal()
                .range(["#E0D75F", "#4682B4", "#779B65", "#CE4F4F", "#AD7FCB", "#4F30B8", "#CB9D5F", "#692432", "#4BF08A", "#B8B79E"]);
    
    var margin = {top: 10, right: 30, bottom: 50, left: 60},
        width =  1200 - margin.left - margin.right,
        height = 500 - margin.top - margin.bottom;

    var x = d3.scale.ordinal()
        .rangeRoundBands([0, width], .1);

    var y = d3.scale.linear()
        .range([height, 0]);
 
    var xAxis = d3.svg.axis()
        .scale(x)
        .orient("bottom")
        .innerTickSize(-height)
        .tickPadding(10)
        .tickValues(xAxisValues)
        .tickFormat(function(d,i){
             return xAxisLabels[i];
                    });
    var yAxis = d3.svg.axis()
        .scale(y)
        .orient("left")
        .innerTickSize(-width)
        .tickPadding(10);

    var prefix = ".chart";   
    var chart = d3.select(prefix.concat(cellNum))
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
      .append("g")
        .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

    x.domain(xyCoords.map(function(d) { return d.x; }));
    y.domain([0, d3.max(xyCoords, function(d) { return d.y; })]);

    chart.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + height + ")")
        .call(xAxis)
      .selectAll("text")
        .attr("x", 25)
        .attr("y", 3)
        .attr("transform", "rotate(45)");

    chart.append("g")
        .attr("class", "y axis")
        .call(yAxis);

    var legend = chart.append("g")
        .attr("class","legend")
        .attr("text-anchor", "end")
      .selectAll("g")
      .data(legends)
      .enter().append("g")
        .attr("transform", function(d, i) { return "translate(0," + i * 20 + ")"; });

    legend.append("rect")
        .attr("x", width - 19)
        .attr("width", 19)
        .attr("height", 19)
        .style("fill", function(d,i){ return color(Math.floor(i%(legends.length)))});

    legend.append("text")
        .attr("x", width - 24)
        .attr("y", 9.5)
        .attr("dy", "0.32em")
        .text(function(d) { return d; });  

    chart.selectAll(".bar")
        .data(xyCoords)
      .enter().append("rect")
        .style("fill", function(d,i){
              return color(Math.floor(i%(legends.length)))})
        .attr("class", "bar")
        .attr("x", function(d) { return x(d.x); })
        .attr("y", function(d) { return y(d.y); })
        .attr("height", function(d) { return height - y(d.y); })
        .attr("width", x.rangeBand());
}

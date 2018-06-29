!function(){
  var viz = { version: "1.4.0" };
  var tau =2*Math.PI, pi=Math.PI, pi2=Math.PI/2;
  viz.maps ={
	  uscountyurl:"https://gist.githubusercontent.com/NPashaP/bf60b406b22a3bdf98f4483fddafdbb5/raw/5d9013e746e916eefe467750f7a11df91a3dc74f/us.json"
  };
  
  viz.biPartite = function(){
	  var key_scale, value_scale
		,keyPrimary, keySecondary, value
		,width, height, orient, barSize, min, pad
		,data, fill, sel, edgeOpacity, duration
		,sortPrimary, sortSecondary, edgeMode
		,refresh=true, bars

	  function biPartite(_){
		sel=_;
		updateLocals()
		bars = biPartite.bars();
		
		sel.select(".viz-biPartite").remove();	
		
		var bp = sel.append("g").attr("class","viz-biPartite")
			
		bp.selectAll(".viz-biPartite-subBar")
        	.data(bars.subBars)
        	.enter()
			.append("g")
			.attr("transform", function(d){ return "translate("+d.x+","+d.y+")";})
        	.attr("class","viz-biPartite-subBar")
        	.append("rect")
        	.attr("x",fx).attr("y",fy).attr("width",fw).attr("height",fh)
			.style("fill", function(d){ return fill(d); })
			 
        bp.selectAll(".viz-biPartite-edge")
        	.data(bars.edges)
        	.enter()
        	.append("path")
        	.attr("class","viz-biPartite-edge")
        	.attr("d",function(d){ return d.path; })
			.style("fill-opacity",biPartite.edgeOpacity())
			.style("fill", function(d){ return fill(d); })
		 
        bp.selectAll(".viz-biPartite-mainBar")
        	.data(bars.mainBars)
        	.enter()
			.append("g")
			.attr("transform", function(d){ return "translate("+d.x+","+d.y+")";})
        	.attr("class","viz-biPartite-mainBar")
        	.append("rect")
        	.attr("x",fx).attr("y",fy).attr("width",fw).attr("height",fh)
			.style("fill-opacity",0)
			.on("mouseover",biPartite.mouseover)
			.on("mouseout",biPartite.mouseout)
	  }
	  biPartite.data = function(_){
		return !arguments.length ? data : (data = _, refresh=true, biPartite);
	  }
	  biPartite.fill = function(_){
		return arguments.length ? (fill = _, refresh=true, biPartite) 
			: fill || (fill=function(){var color = d3.scaleOrdinal(d3.schemeCategory10);  return function(d){ return color(d.primary);}}());		
	  }
	  biPartite.keyPrimary = function(_){ 
		return arguments.length ? (keyPrimary = _, refresh=true, biPartite) : keyPrimary || (keyPrimary=function(d){ return d[0] });
	  }
	  biPartite.sortPrimary = function(_){ 
		return arguments.length ? (sortPrimary = _, refresh=true, biPartite) : sortPrimary || (sortPrimary=d3.ascending) ;
	  }
	  biPartite.keySecondary = function(_){ 
		return arguments.length ? (keySecondary = _, refresh=true, biPartite) : keySecondary||(keySecondary=function(d){ return d[1] });
	  }
	  biPartite.sortSecondary = function(_){ 
		return arguments.length ? (sortSecondary = _, refresh=true, biPartite) : sortSecondary || (sortSecondary=d3.ascending);	
	  }	  
	  biPartite.value = function(_){ 
		return arguments.length ? (value = _, refresh=true, biPartite) : value || (value=function(d){ return d[2] });
	  }	  
	  biPartite.width = function(_){
		return arguments.length ? (width = _, refresh=true, biPartite) : width || (width=(biPartite.orient()=="vertical" ? 400: 600));
	  }
	  biPartite.height = function(_){
		return arguments.length ? (height = _, refresh=true, biPartite) : height|| (height=(biPartite.orient()=="vertical" ? 600: 400));
	  }
	  biPartite.barSize = function(_){
		return arguments.length ? (barSize = _, refresh=true, biPartite) : barSize || (barSize=35);
	  }
	  biPartite.min = function(_){
		return arguments.length ? (min = _, refresh=true, biPartite) : typeof min=="undefined" ? (min=0): min;
	  }
	  biPartite.orient = function(_){
		return arguments.length ? (orient = _, refresh=true, biPartite) : typeof orient=="undefined" ? (orient="vertical"):orient;
	  }
	  biPartite.pad = function(_){
		return arguments.length ? (pad = _, refresh=true, biPartite) : typeof pad=="undefined" ? (pad=1): pad;
	  }
	  biPartite.duration = function(_){
		return arguments.length ? (duration = _, refresh=true, biPartite) : typeof duration=="undefined" ? (duration=500):duration;
	  }
	  biPartite.edgeOpacity = function(_){
		return arguments.length ? (edgeOpacity = _, refresh=true, biPartite) : typeof edgeOpacity=="undefined" ? (edgeOpacity=.4):edgeOpacity;
	  }
	  biPartite.edgeMode = function(_){
		return arguments.length ? (edgeMode = _, refresh=true, biPartite) : edgeMode || (edgeMode="curved");
	  }
	  biPartite.bars = function(mb){
		var mainBars={primary:[], secondary:[]};
		var subBars= {primary:[], secondary:[]};
		var key ={primary:biPartite.keyPrimary(), secondary:biPartite.keySecondary() };
		var _or = biPartite.orient();
		
		calculateMainBars("primary");
		calculateMainBars("secondary");	
		calculateSubBars("primary");	
		calculateSubBars("secondary");
		floorMainBars(); // ensure that main bars is atleast of size mi.n
		
		return {
			 mainBars:mainBars.primary.concat(mainBars.secondary)
			,subBars:subBars.primary.concat(subBars.secondary)
			,edges:calculateEdges()
		};

		function isSelKey(d, part){ 
			return (typeof mb === "undefined" || mb.part === part) || (key[mb.part](d) === mb.key);
		}
		function floorMainBars(){
			var m =biPartite.min()/2;
			
			mainBars.primary.forEach(function(d){
				if(d.height<m) d.height=m;
			});
			mainBars.secondary.forEach(function(d){
				if(d.height<m) d.height=m;
			});
		}
		function calculateMainBars(part){
			function v(d){ return isSelKey(d, part) ? biPartite.value()(d): 0;}

			var ps = d3.nest()
				.key(part=="primary"? biPartite.keyPrimary():biPartite.keySecondary())
				.sortKeys(part=="primary"? biPartite.sortPrimary():biPartite.sortSecondary())
				.rollup(function(d){ return d3.sum(d,v); })
				.entries(data)
			
			var bars = bpmap(ps, biPartite.pad(), biPartite.min(), 0, _or=="vertical" ? biPartite.height() : biPartite.width())
			var bsize = biPartite.barSize()
			ps.forEach(function(d,i){ 
				mainBars[part].push({
					 x:_or=="horizontal"? (bars[i].s+bars[i].e)/2 : (part=="primary" ? bsize/2 : biPartite.width()-bsize/2)
					,y:_or=="vertical"? (bars[i].s+bars[i].e)/2 : (part=="primary" ? bsize/2 : biPartite.height()-bsize/2)
					,height:_or=="vertical"? (bars[i].e - bars[i].s)/2 : bsize/2
					,width: _or=="horizontal"? (bars[i].e - bars[i].s)/2 : bsize/2
					,part:part
					,key:d.key
					,value:d.value
					,percent:bars[i].p
				});
			});
		}
		function calculateSubBars(part){
			function v(d){ return isSelKey(d, part) ? biPartite.value()(d): 0;};
			
			var sort = part=="primary"
					? function(a,b){ return biPartite.sortPrimary()(a.key, b.key);}
					: function(a,b){ return biPartite.sortSecondary()(a.key, b.key);}
					
			var map = d3.map(mainBars[part], function(d){ return d.key});
			
			var ps = d3.nest()
				.key(part=="primary"? biPartite.keyPrimary():biPartite.keySecondary())
				.sortKeys(part=="primary"? biPartite.sortPrimary():biPartite.sortSecondary())
				.key(part=="secondary"? biPartite.keyPrimary():biPartite.keySecondary())
				.sortKeys(part=="secondary"? biPartite.sortPrimary():biPartite.sortSecondary())
				.rollup(function(d){ return d3.sum(d,v); })
				.entries(biPartite.data());
	
			ps.forEach(function(d){ 
				var g= map.get(d.key); 
				var bars = bpmap(d.values, 0, 0
						,_or=="vertical" ? g.y-g.height : g.x-g.width
						,_or=="vertical" ? g.y+g.height : g.x+g.width);

				var bsize = biPartite.barSize();			
				d.values.forEach(function(t,i){ 
					subBars[part].push({
						 x:_or=="vertical"? part=="primary" ? bsize/2 : biPartite.width()-bsize/2 : (bars[i].s+bars[i].e)/2
						,y:_or=="horizontal"? part=="primary" ? bsize/2 : biPartite.height()-bsize/2 : (bars[i].s+bars[i].e)/2
						,height:(_or=="vertical"? bars[i].e - bars[i].s : bsize)/2
						,width: (_or=="horizontal"? bars[i].e - bars[i].s : bsize)/2
						,part:part
						,primary:part=="primary"? d.key : t.key
						,secondary:part=="primary"? t.key : d.key	
						,value:t.value
						,percent:bars[i].p*g.percent
						,index: part=="primary"? d.key+"|"+t.key : t.key+"|"+d.key //index 
					});
				});		  
			});
		}
		function calculateEdges(){	
			var map=d3.map(subBars.secondary,function(d){ return d.index;});
			var eMode= biPartite.edgeMode();
			
			return subBars.primary.map(function(d){
				var g=map.get(d.index);
				return {
					 path:_or === "vertical" 
						? edgeVert(d.x+d.width,d.y+d.height,g.x-g.width,g.y+g.height,g.x-g.width,g.y-g.height,d.x+d.width,d.y-d.height)
						: edgeHoriz(d.x-d.width,d.y+d.height,g.x-g.width,g.y-g.height,g.x+g.width,g.y-g.height,d.x+d.width,d.y+d.height)
					,primary:d.primary
					,secondary:d.secondary
					,value:d.value
					,percent:d.percent
				}
			});
			function edgeVert(x1,y1,x2,y2,x3,y3,x4,y4){
				if(eMode=="straight") return ["M",x1,",",y1,"L",x2,",",y2,"L",x3,",",y3,"L",x4,",",y4,"z"].join("")
				var mx1=(x1+x2)/2,mx3=(x3+x4)/2;
				return ["M",x1,",",y1,"C",mx1,",",y1," ",mx1,",",y2,",",x2,",",y2,"L"
						,x3,",",y3,"C",mx3,",",y3," ",mx3,",",y4,",",x4,",",y4,"z"].join("");
			}
			function edgeHoriz(x1,y1,x2,y2,x3,y3,x4,y4){
				if(eMode=="straight") return ["M",x1,",",y1,"L",x2,",",y2,"L",x3,",",y3,"L",x4,",",y4,"z"].join("")
				var my1=(y1+y2)/2,my3=(y3+y4)/2;
				return ["M",x1,",",y1,"C",x1,",",my1," ",x2,",",my1,",",x2,",",y2,"L"
						,x3,",",y3,"C",x3,",",my3," ",x4,",",my3,",",x4,",",y4,"z"].join("");
			}
		}
		function bpmap(a/*array*/, p/*pad*/, m/*min*/, s/*start*/, e/*end*/){
			var r = m/(e-s-2*a.length*p); // cut-off for ratios
			var ln =0, lp=0, t=d3.sum(a,function(d){ return d.value;}); // left over count and percent.
			a.forEach(function(d){ if(d.value < r*t ){ ln+=1; lp+=d.value; }})
			var o= t < 1e-5 ? 0:(e-s-2*a.length*p-ln*m)/(t-lp); // scaling factor for percent.
			var b=s, ret=[];
			a.forEach(function(d){ 
				var v =d.value*o; 
				ret.push({
					 s:b+p+(v<m?.5*(m-v): 0)
					,e:b+p+(v<m? .5*(m+v):v)
					,p:t < 1e-5? 0:d.value/t
				}); 
				b+=2*p+(v<m? m:v); 
			});	
			return ret;
		}
	  }	  
	  biPartite.update = function(_data){
	    data = _data;
		updateLocals()
		
		bars = biPartite.bars()
		
		sel.selectAll(".viz-biPartite-subBar")
			.data(bars.subBars)
			.transition()
			.duration(duration)
			.attr("transform", function(d){ return "translate("+d.x+","+d.y+")";})
        	.select("rect")
        	.attr("x",fx)
			.attr("y",fy)
			.attr("width",fw)
			.attr("height",fh);
						 
        sel.selectAll(".viz-biPartite-edge")
			.data(bars.edges)
			.transition()
			.duration(duration)
        	.attr("d",function(d){ return d.path; })
			.style("fill-opacity",biPartite.edgeOpacity())
        	 
        sel.selectAll(".viz-biPartite-mainBar")
			.data(bars.mainBars)
			.transition()
			.duration(duration)
			.attr("transform", function(d){ return "translate("+d.x+","+d.y+")";})
        	.select("rect")
        	.attr("x",fx)
			.attr("y",fy)
			.attr("width",fw)
			.attr("height",fh)
			.style("fill-opacity",0)
			
		return biPartite
	  }
	  biPartite.mouseover = function(d){
		  var newbars = biPartite.bars(d)
		  
		  sel.selectAll(".viz-biPartite-mainBar")
			.filter(function(r){ return r.part===d.part && r.key === d.key})
			.select("rect")
			.style("stroke-opacity", 1)
		  
		  sel.selectAll(".viz-biPartite-subBar")
			.data(newbars.subBars)
			.transition()
			.duration(biPartite.duration())
			.attr("transform", function(d){ return "translate("+d.x+","+d.y+")";})
			.select("rect")
			.attr("x",fx)
			.attr("y",fy)
			.attr("width",fw)
			.attr("height",fh)
			
		  var e = sel.selectAll(".viz-biPartite-edge")
			.data(newbars.edges)
			
		  e.filter(function(t){ return t[d.part] === d.key;})
			.transition()
			.duration(biPartite.duration())
			.style("fill-opacity",biPartite.edgeOpacity())
			.attr("d",function(d){ return d.path});	
			
		  e.filter(function(t){ return t[d.part] !== d.key;})
			.transition()
			.duration(biPartite.duration())
			.style("fill-opacity",0)
			.attr("d",function(d){ return d.path})
			
		  sel.selectAll(".viz-biPartite-mainBar")
			.data(newbars.mainBars)
			.transition()
			.duration(biPartite.duration())
			.attr("transform", function(d){ return "translate("+d.x+","+d.y+")";})
			.select("rect")
			.attr("x",fx)
			.attr("y",fy)
			.attr("width",fw)
			.attr("height",fh)
		}
	  biPartite.mouseout = function(d){					  
		  sel.selectAll(".viz-biPartite-mainBar")
			.filter(function(r){ return r.part===d.part && r.key === d.key})
			.select("rect")
			.style("stroke-opacity", 0)
		  
		  sel.selectAll(".viz-biPartite-subBar")
			.data(bars.subBars)
			.transition()
			.duration(biPartite.duration())
			.attr("transform", function(d){ return "translate("+d.x+","+d.y+")";})
			.select("rect")
			.attr("x",fx)
			.attr("y",fy)
			.attr("width",fw)
			.attr("height",fh)
			
		  sel.selectAll(".viz-biPartite-edge")
			.data(bars.edges)
			.transition()
			.duration(biPartite.duration())
			.style("fill-opacity",biPartite.edgeOpacity())
			.attr("d",function(d){ return d.path})
			
		  sel.selectAll(".viz-biPartite-mainBar")
			.data(bars.mainBars)
			.transition()
			.duration(biPartite.duration())
			.attr("transform", function(d){ return "translate("+d.x+","+d.y+")";})
			.select("rect")
			.attr("x",fx)
			.attr("y",fy)
			.attr("width",fw)
			.attr("height",fh)
		}
	  function updateLocals(){
		if(!refresh) return;
		biPartite.fill();
		biPartite.keyPrimary();
		biPartite.sortPrimary();
		biPartite.keySecondary();
		biPartite.sortSecondary();
		biPartite.value();
		biPartite.width();	
		biPartite.height();	
		biPartite.barSize();		
		biPartite.min();		
		biPartite.orient();		
		biPartite.pad();		
		biPartite.duration();		
		biPartite.edgeOpacity();		
		biPartite.edgeMode();
		refresh=false;
	  }
	  function fx(d){ return -d.width}
	  function fy(d){ return -d.height}
      function fw(d){ return 2*d.width}
      function fh(d){ return 2*d.height}
	  
	  return biPartite;
	}
  viz.gg = function(){
	  var innerRadius, outerRadius, startAngle, endAngle, needleColor, innerFaceColor, faceColor
		, domain, value, angleOffset, duration, ease, g, dpg, ticks, majorTicks
		, minorTickStart, minorTickEnd, majorTickStart, majorTickEnd, labelLocation
		
	  var def={
		innerRadius:20, outerRadius:150, angleOffset:0.7
		,startAngle:-1.5*pi, endAngle:0.5*pi
		,minorTickStart:.9, minorTickEnd:.95, majorTickStart:.82, majorTickEnd:.95
		,needleColor:"#de2c2c", innerFaceColor:"#999999", faceColor:"#666666"
		,domain:[0,100], duration:500, ease:"cubicInOut"
		,ticks:d3.range(0,101,2), majorTicks: function(d){ return d%10===0}
		,labelLocation: .7
	  };
	  function gg(_){
		g=_;
        _.each(function() {
			var g = d3.select(this);
			var a = gg.scale();
			var mS=gg.minorTickStart(), mE=gg.minorTickEnd(),MS=gg.majorTickStart(), ME=gg.majorTickEnd();
			var ticks=gg.ticks(), mT=gg.majorTicks(), lL=gg.labelLocation();
			var or = gg.outerRadius();
			
			g.append("circle").attr("r",or)
				.style("fill","url(#vizgg3"+dpg+")")
				.attr("class","face");
	
			g.append("circle").attr("r",gg.innerRadius())
				.style("fill","url(#vizgg2"+dpg+")")
				.style("filter","url(#vizgg5"+dpg+")")
				.attr("class","innerFace");
  
			var tickg = g.append("g");
			tickg.selectAll("line").data(ticks).enter().append("line")
				.attr("class",function(d){ return mT(d) ? "majorTicks": "minorTicks" })
				.attr("x1",function(d){ return or*(mT(d)? MS:mS)*Math.cos(a(d));})
				.attr("y1",function(d){ return or*(mT(d)? MS:mS)*Math.sin(a(d));})
				.attr("x2",function(d){ return or*(mT(d)? ME:mE)*Math.cos(a(d));})
				.attr("y2",function(d){ return or*(mT(d)? ME:mE)*Math.sin(a(d));});
  
			g.selectAll("text").data(ticks.filter(mT))
				.enter().append("text").attr("class","label")
				.attr("x",function(d){ return or*lL*Math.cos(a(d));})
				.attr("y",function(d){ return or*lL*Math.sin(a(d));})
				.attr("dy",3)
				.text(function(d){ return d;});
				
			var r = gg.outerRadius()/def.outerRadius;

			var rot=gg.scale()(gg.value())*180/pi+90;
			
			g.append("g").attr("transform","translate(1,1)")
				.selectAll(".needleshadow").data([0]).enter().append("g")
				.attr("transform","rotate("+rot+")")
				.attr("class","needleshadow")
				.append("path")
				.attr("d",["m 0",-130*r, 5*r, 175*r, -10*r, "0,z"].join(","))
				.style("filter","url(#vizgg6"+dpg+")");
	
			g.selectAll(".needle").data([0]).enter().append("g")
				.attr("transform","rotate("+rot+")")
				.attr("class","needle")
				.append("polygon")
				.attr("points",[-0.5*r,-130*r, 0.5*r,-130*r, 5*r,45*r, -5*r,45*r].join(","))
				.style("fill","url(#vizgg4"+dpg+")");			
		});		  
	  }
	  gg.scale = function(){ 
		return d3.scale.linear().domain(gg.domain())
			.range([def.startAngle+gg.angleOffset(), def.endAngle -gg.angleOffset()]);
	  }
	  gg.innerRadius = function(_){
		if(!arguments.length) return typeof innerRadius !== "undefined" ? innerRadius : def.innerRadius;
		innerRadius = _;
		return gg;
	  }
	  gg.outerRadius = function(_){
		if(!arguments.length) return typeof outerRadius !== "undefined" ? outerRadius : def.outerRadius;
		outerRadius = _;
		return gg;
	  }
	  gg.angleOffset = function(_){
		if(!arguments.length) return typeof angleOffset !== "undefined" ? angleOffset : def.angleOffset;
		angleOffset = _;
		return gg;
	  }
	  gg.labelLocation = function(_){
		if(!arguments.length) return typeof labelLocation !== "undefined" ? labelLocation : def.labelLocation;
		labelLocation = _;
		return gg;
	  }
	  gg.ticks = function(_){
		if(!arguments.length) return typeof ticks !== "undefined" ? ticks : def.ticks;
		ticks = _;
		return gg;
	  }
	  gg.majorTicks = function(_){
		if(!arguments.length) return typeof majorTicks !== "undefined" ? majorTicks : def.majorTicks;
		majorTicks = _;
		return gg;
	  }
	  gg.minorTickStart = function(_){
		if(!arguments.length) return typeof minorTickStart !== "undefined" ? minorTickStart : def.minorTickStart;
		minorTickStart = _;
		return gg;
	  }
	  gg.minorTickEnd = function(_){
		if(!arguments.length) return typeof minorTickEnd !== "undefined" ? minorTickEnd : def.minorTickEnd;
		minorTickEnd = _;
		return gg;
	  }
	  gg.majorTickStart = function(_){
		if(!arguments.length) return typeof majorTickStart !== "undefined" ? majorTickStart : def.majorTickStart;
		majorTickStart = _;
		return gg;
	  }
	  gg.majorTickEnd = function(_){
		if(!arguments.length) return typeof majorTickEnd !== "undefined" ? majorTickEnd : def.majorTickEnd;
		majorTickEnd = _;
		return gg;
	  }
	  gg.needleColor = function(_){
		if(!arguments.length) return typeof needleColor !== "undefined" ? needleColor : def.needleColor;
		needleColor = _;
		return gg;
	  }
	  gg.innerFaceColor = function(_){
		if(!arguments.length) return typeof innerFaceColor !== "undefined" ? innerFaceColor : def.innerFaceColor;
		innerFaceColor = _;
		return gg;
	  }
	  gg.faceColor = function(_){
		if(!arguments.length) return typeof faceColor !== "undefined" ? faceColor : def.faceColor;
		faceColor = _;
		return gg;
	  }
	  gg.domain = function(_){
		if(!arguments.length) return typeof domain !== "undefined" ? domain : def.domain;
		domain = _;
		return gg;
	  }
	  gg.duration = function(_){
		if(!arguments.length) return typeof duration !== "undefined" ? duration : def.duration;
		duration = _;
		return gg;
	  }
	  gg.ease = function(_){
		if(!arguments.length) return typeof ease !== "undefined" ? ease : def.ease;
		ease = _;
		return gg;
	  }
	  gg.value = function(_){
		if(!arguments.length) return typeof value !== "undefined" ? value : .5*(def.domain[0]+def.domain[1]);
		value = _;
		return gg;
	  }
	  gg.defs = function(svg, dg){

		var defs=svg.append("defs");
		dpg=dg;
		var nc = gg.needleColor();
		var fc =gg.innerFaceColor();
		var fbc =gg.faceColor();
		
		var lg1 =viz.defs(defs).lG().id("vizgg1"+dg).sel();		
		viz.defs(lg1).stop().offset("0").stopColor(nc);
		viz.defs(lg1).stop().offset("1").stopColor(d3.rgb(nc).darker(1));
		
		var rg1 =viz.defs(defs).rG().id("vizgg2"+dg)
			.fx("35%").fy("65%").r("65%").spreadMethod("pad").sel();
		viz.defs(rg1).stop().offset("0").stopColor(fc);
		viz.defs(rg1).stop().offset("1").stopColor(d3.rgb(fc).darker(2));
		
		var rg2 =viz.defs(defs).rG().id("vizgg3"+dg)
			.fx("35%").fy("65%").r("65%").spreadMethod("pad").sel();
		viz.defs(rg2).stop().offset("0").stopColor(fbc);
		viz.defs(rg2).stop().offset("1").stopColor(d3.rgb(fbc).darker(2));
		
		viz.defs(defs).lG().id("vizgg4"+dg).gradientUnits("userSpaceOnUse")
			.y1("80").x1("-10").y2("80").x2("10").xlink("#vizgg1"+dg)
			
		var fl1 = viz.defs(defs).filter().id("vizgg5"+dg).sel();
		viz.defs(fl1).feFlood().result("flood").floodColor("rgb(0,0,0)").floodOpacity("0.6");
		viz.defs(fl1).feComposite().result("composite1").operator("in").in2("SourceGraphic").in("flood");
		viz.defs(fl1).feGaussianBlur().result("blur").stdDeviation("2").in("composite1");
		viz.defs(fl1).feOffset().result("offset").dy("2").dx("2");
		viz.defs(fl1).feComposite().result("composite2").operator("over").in2("offset").in("SourceGraphic");
			
		var fl2 =viz.defs(defs).filter().x("-0.3").y("-0.3").height("1.8").width("1.8").id("vizgg6"+dg).sel();
		viz.defs(fl2).feGaussianBlur().stdDeviation("2");
	  }
	  
	  gg.setNeedle =function(a){
		var newAngle=gg.scale()(a)*180/pi+90
			,oldAngle=gg.scale()(gg.value())*180/pi+90
			,d3ease = gg.ease()
			;
		g.selectAll(".needle").data([a])
			.transition().duration(gg.duration())
			.attrTween("transform",function(d){ return iS(oldAngle,newAngle); })
			.ease(d3ease);
		
		g.selectAll(".needleshadow").data([a])
			.transition().duration(gg.duration())
			.attrTween("transform",function(d){  return iS(oldAngle,newAngle); })
			.ease(d3ease)
			.each("end",function(){angle=a;});
			
		gg.value(a);
		
		function iS(o,n){
			return d3.interpolateString("rotate("+o+")", "rotate("+n+")");
		}
	  }
	  
	  return gg;
  }
  viz.chord = function(){
    var   data, fill, duration, chordOpacity, innerRadius, outerRadius
		, source, target, value, padAngle, labelPadding, sort
		, startAngle, chords, groups, label, min=0
		, sel, chords, newchords, groups, newgroups
    	
  function chord(_){  
    sel=_;
	
    relayout();

	sel.select(".viz-chord").remove()
	
    var gchord = sel.append("g").attr("class","viz-chord")
	 	
	gchord
		.append("g")
		.attr("class","viz-chord-groups")
		.selectAll(".group")
		.data(groups)
        .enter()
		.append("path")
		.attr("class","group")
		.on("mouseover",chord.mouseover)
		.on("mouseout",chord.mouseout)	 
		.style("fill", function(d){ return fill(d.source)})
		.style("stroke", function(d){ return fill(d.source)})
		.attr("d", arc)
		.each(function(d) { this._current = d; })
		  
    gchord
		.append("g")
		.attr("class", "viz-chord-chords")
		.selectAll(".chord")
		.data(chords)
		.enter()
		.append("path")
		.attr("class","chord")
		.each(function(d) { this._current = d; })
		.attr("d", _chord)
		.style("fill", function(d){ return fill(d.target)})
		.style("opacity", chordOpacity)
		.style("stroke", function(d){ return fill(d.target)})
		.style("display",function(d){ return d.display ? "inline" : "none";})

	gchord
		.append("g")
		.attr("class","viz-chord-labels")
		.selectAll(".label")
		.data(groups.filter(function(d){ return d.type=="g"}))
		.enter()
		.append("text")
		.attr("class","label")	
		.on("mouseover",chord.mouseover)
		.on("mouseout",chord.mouseout)			 
		.attr("x",function(d){ return d.labelx})
		.attr("y",function(d){ return d.labely})
		.text(label)
		.style("text-anchor",function(d){var a =angle(d); return a < pi2 || a>tau-pi2 ? "start" : "end";})
		.each(function(d) { this._current = d; })		
	
	function arc(d){
	  return viz_arc([innerRadius, outerRadius, d.startAngle, d.endAngle]);
	}
	function _chord(d){
	  return viz_chord(innerRadius, d.startAngle, d.endAngle, 
	                   innerRadius, d.endStartAngle, d.endEndAngle);
	}	
  }
  chord.data = function(_){
	return !arguments.length ? data : (data = _, reComputeLayout=true, chord);
  }
  chord.fill = function(_){
	return arguments.length ? (fill = _, chord) 
	: typeof fill !== "undefined" ? fill : (fill=viz_schemeCategory10()) ;
  }
  chord.duration = function(_){
	return arguments.length ? (duration = _, chord) :  typeof duration !== "undefined" ? duration : (duration=500) ;
  }
  chord.chordOpacity = function(_){
	return arguments.length ? (chordOpacity = _, chord) :  typeof chordOpacity !== "undefined" ? chordOpacity : (chordOpacity=.7) ;
  }
  chord.innerRadius = function(_){
	return arguments.length ? (innerRadius = _, reComputeLayout=true, chord) :  typeof innerRadius !== "undefined" ? innerRadius : (innerRadius=180);
  }
  chord.outerRadius = function(_){
	return arguments.length ? (outerRadius = _, reComputeLayout=true, chord) :  typeof outerRadius !== "undefined" ? outerRadius : (outerRadius=200);
  }
  chord.source = function(_){ 
	return arguments.length ? (source = _, reComputeLayout=true, chord) 
		:  typeof source !== "undefined" ? source : (source=function(d){ return d[0];});
  }
  chord.target = function(_){ 
	return arguments.length ? (target = _, reComputeLayout=true, chord) 
		:  typeof target !== "undefined" ? target : (target=function(d){ return d[1];});   
  }
  chord.value = function(_){ 
	return arguments.length ? (value = _, reComputeLayout=true, chord) 
		:  typeof value !== "undefined" ? value : (value=function(d){ return d[2];});  
  }
  chord.padAngle = function(_){ 
	return arguments.length ? (padAngle = _, reComputeLayout=true, chord) 
		:  typeof padAngle !== "undefined" ? padAngle : (padAngle=0.03); 
  }
  chord.labelPadding = function(_){ 
	return arguments.length ? (labelPadding = _, chord) 
		:  typeof labelPadding !== "undefined" ? labelPadding : (labelPadding=1.02); 
  }
  chord.sort = function(_){ 
	return arguments.length ? (sort = _, reComputeLayout=true, chord) 
		:  typeof sort !== "undefined" ? sort : (sort=d3.ascending); 
  }
  chord.startAngle = function(_){ 
	return arguments.length ? (startAngle = _, reComputeLayout=true, chord) 
		:  typeof startAngle !== "undefined" ? startAngle : (startAngle=0); 		
  }
  chord.chords = function(){
      if (reComputeLayout) relayout();
      return chords;
  }
  chord.groups = function(){
      if (reComputeLayout) relayout();
      return groups;
  }
  chord.label = function(_){
	return arguments.length ? (label = _, chord) 
		:  typeof label !== "undefined" ? label : (label=function(d){ return d.source+" ("+d.value+")"}); 
  }
  chord.mouseover = function(d){
    relayouts(d.source);
    transition(1);
  }
  chord.mouseout = function(d){
    transition(0);
  }
  chord.update = function(_data){
    data = _data;	  
	relayout();
	transition(0)
  }
  function updateLocals(){
	chord.source()
	chord.target()
	chord.sort()
	chord.duration()
	chord.chordOpacity()
	chord.innerRadius()
	chord.outerRadius()
	chord.value()
	chord.padAngle()
	chord.labelPadding()
	chord.sort()
	chord.startAngle()
	chord.label()	
	chord.fill()
  }
  function relayout(){
	keys =[];
	updateLocals()
			  
	data.forEach(function(d){ 
	  if(keys.indexOf(source(d))==-1) keys.push(source(d));
	  if(keys.indexOf(target(d))==-1) keys.push(target(d));
	});
	keys =keys.sort(sort);
	
	subgrp = {}; 
	chordExist={};
	keys.forEach(function(k){ 
	  subgrp[k]={}; chordExist[k]={}; 
	  keys.forEach(function(l){ subgrp[k][l]=0; chordExist[k][l]=false;})
	});
	
	data.forEach(function(d){ var s =source(d), t=target(d); subgrp[s][t]+=value(d); chordExist[s][t] =true;});
	
	groups=[];

	keys.forEach(function(k,i){ 
	  groups.push({
	     source:k
		,type:"gs" // group to itself
		,value:0
		,skipPad:true
		,index:i
	  });
	  groups.push({
	     source:k
		,type:"g" //group to outside
		,value:d3.sum(keys, function(d){ return subgrp[k][d]})
		,skipPad:false
		,index:i
	  });
	});
	viz_circularPartition(groups, padAngle, min, undefined, startAngle);
	
	chords=[];
	groups.filter(function(g){ return g.type=="g"})
	  .forEach(function(g, gi){
		var _labelangle = angle(g)		
		g.labelx = labelPadding*outerRadius*Math.cos(_labelangle)
		g.labely = labelPadding*outerRadius*Math.sin(_labelangle)
		
        var gia = viz_shiftarray(keys.length,gi);
	    
	    var grpbarsgia = viz_getbars(gia.map(function(d){return subgrp[g.source][keys[d]];}), 0, 0, g.startAngle, g.endAngle);
	    
	    gia.forEach(function(si, i){		
	      var t1=grpbarsgia[i];
  	        chords.push({
	  		         startAngle:t1.c-t1.v/2
	                 , endAngle:t1.c+t1.v/2
	  			   , value:t1.value
	  			   , source:g.source
	  			   , target:keys[si]
	  			   , type:"c"
	  			   , display:chordExist[g.source][keys[si]]
	  			   , index:gi
	  			   , subindex:si
	  			   , indexsubindex:gi+"-"+si
	  	  });
	    });
	  });
	var m = d3.map(chords, function(d){ return d.indexsubindex;});
	chords.forEach(function(d){ 
	  if(d.subindex == d.index){
		d.endStartAngle=d.startAngle; d.endEndAngle=d.startAngle;
		return;
	  }
	  var z= m.get(d.subindex+"-"+d.index); 
	  d.endStartAngle=z.startAngle; 
	  d.endEndAngle=z.startAngle;
	});	
	reComputeLayout=false;
  }
  function relayouts(fixedSource){
	var fg = groups.filter(function(g){ return g.source==fixedSource && g.type =="g"})[0]; 
	newgroups=[];

	keys.forEach(function(k,i){ 
	  newgroups.push({
	     source:k
	    ,startAngle:fg.startAngle
		,endAngle:fg.endAngle
		,padAngle:fg.padAngle
		,percent:fg.percent
		,type:"gs"
		,value:k==fixedSource ? subgrp[k][k] : 0
		,skipPad: k==fixedSource && chordExist[k][k] ? false : true
		,index:i
	  });
	  if(k==fixedSource)
  	    newgroups.push({
	       source:k
	      ,startAngle:fg.startAngle
		  ,endAngle:fg.endAngle
		  ,padAngle:fg.padAngle
		  ,percent:fg.percent
		  ,type:"g"
		  ,value:fg.value
		  ,skipPad:false
		  ,index:i
	    });
	  else 
  	    newgroups.push({
	       source:k
		  ,type:"g"
		  ,value:subgrp[fixedSource][k]
		  ,skipPad:false
		  ,index:i
	    });
	});
	
	viz_circularPartition(newgroups, padAngle, min, fixedSource, startAngle);
	
	function mid(z){ return (z.endAngle+z.startAngle) }
	var sm = mid(fg);
	groups.forEach(function(g,i){
	  var g1=newgroups[i];
	  var f = (mid(g) < sm) ;
	  g1.startAngle-=(f ? tau:0);
	  g1.endAngle -= (f ? tau:0); 
	});
	
	newchords=[];
	newgroups.filter(function(g){ return g.type=="g"}).forEach(function(g, gi){
	  var _labelangle = angle(g)		
	  g.labelx = labelPadding*outerRadius*Math.cos(_labelangle)
	  g.labely = labelPadding*outerRadius*Math.sin(_labelangle)
	  
      var gia = viz_shiftarray(keys.length,gi);
	  
	  var a0 = gia.map(function(d){ var k = keys[d]; 
	    return g.source== fixedSource ? subgrp[g.source][k] : k==fixedSource ? subgrp[k][g.source]:0;
	  });
	  
	  var grpbarsgia = viz_getbars(a0, 0, 0,g.startAngle, g.endAngle);
	  
	  gia.forEach(function(si, i){		
	    var t1=grpbarsgia[i];
		newchords.push({startAngle:t1.c-t1.v/2
	               , endAngle:t1.c+t1.v/2
				   , value:t1.value
				   , source:g.source
				   , target:keys[si]
				   , type:"c"
				   , display:g.source === fixedSource
				   , index:gi
				   , subindex:si
				   , indexsubindex:gi+"-"+si
		           });
	  });
	});
	var m = d3.map(newchords.map(function(d){ return {startAngle:d.startAngle, endAngle:d.endAngle, indexsubindex:d.indexsubindex};})
	     , function(d){ return d.indexsubindex;});
	var gmap = d3.map(newgroups.filter(function(d){ return d.type=="gs"}),function(d){ return d.source;});
	
	newchords.forEach(function(d){ 
	  if(d.subindex == d.index){
		var g0 = gmap.get(d.source);
		d.endStartAngle=g0.startAngle; d.endEndAngle=g0.endAngle;
		return;
	  }
	  var z= m.get(d.subindex+"-"+d.index); 
	  d.endStartAngle=z.startAngle; 
	  d.endEndAngle=z.endAngle;		  
	  if(d.source!==fixedSource){
		  d.startAngle=d.endAngle;
		  d.endEndAngle = d.endStartAngle;
	  }
	});
  }
  function transition(f){
    function arc(d,t){
	  return viz_arc([innerRadius, outerRadius, d.startAngle, d.endAngle]);
	}
	function _chord(d){
	  return viz_chord(innerRadius, d.startAngle, d.endAngle, 
	                   innerRadius, d.endStartAngle, d.endEndAngle);
	}
    function chordTween(a) {
      var i = d3.interpolate(this._current, a);
      this._current = i(0);
      return function(t) { return _chord(i(t)); };
    }  
    function arcTween(a) {
      var i = d3.interpolate(this._current, a);
      this._current = i(0);
      return function(t) { return arc(i(t),t); };
    }  
    function labelTweenx(a) {
      var i = d3.interpolate(this._current, a);
      this._current = i(0);
      return function(t) { return labelPadding*outerRadius*Math.cos(angle(i(t))); };
    }  
    function labelTweeny(a) {
      var i = d3.interpolate(this._current, a);
      this._current = i(0);
      return function(t) { return labelPadding*outerRadius*Math.sin(angle(i(t))); };
    }  
	
    var gchord = sel.select(".viz-chord")
	var tempgroups = f? newgroups : chord.groups()
	
	gchord.select(".viz-chord-groups")
		.selectAll(".group")
		.data(tempgroups)
		.transition()
		.duration(duration)
		.attrTween("d", arcTween)
	
	gchord.select(".viz-chord-chords")
		.selectAll(".chord")
		.data(f? newchords : chord.chords())
		.transition()
		.duration(duration)
		.attrTween("d", chordTween)
		.style("opacity",function(d){ return d.display ? chordOpacity : 0;});
	
	
	gchord.select(".viz-chord-labels")
		.selectAll(".label")
		.data(tempgroups.filter(function(d){ return d.type=="g"}))
	    .transition()
		.duration(duration)
	    .attrTween("x",labelTweenx)
	    .attrTween("y",labelTweeny)
		.text(label)
		.style("text-anchor",function(d){var a =angle(d); return a < pi2 || a>tau-pi2 ? "start" : "end";});	
  }
  function angle(d){
    return viz_reduceAngle((d.startAngle+d.endAngle)/2);
  }
  function viz_circularPartition(data, pad, min, fixedSource, startAngle){
    var fixed = (fixedSource !== undefined), ind=0;
	  
    if(fixed){
      var found=false;
	  for(;ind<data.length; ind++){ 
	    if(data[ind].source ==fixedSource && data[ind].type == "g") {found=true; break;}
	  }
	  if(!found) console.log("The fixed source '" +fixedSource+"' is not a valid key" );
	}  
    var dorder = d3.range(data.length);
	if(fixed) dorder =dorder.slice(ind).concat(dorder.slice(0,ind));
	  
    var a0 = data.filter(function(d){ return (!fixed || d.source!==fixedSource  || d.type!="g") && !d.skipPad})
	  .map(function(d){ return d.value;});
	  
	var ta = 2*Math.PI - (fixed ? (data[ind].endAngle-data[ind].startAngle +2*pad): 0);
  
    var x=(fixed ? data[ind].endAngle +pad: startAngle), total = d3.sum(a0);
    var r=viz_getratio(a0, pad, min, ta<=0 ? 0 : ta, total, fixed ? true: false);
	
    dorder.slice(fixed ? 1:0).forEach(function(i){
      var v = r*data[i].value;
      var w =(v < min ? min-v :0)/2;
	
      data[i].startAngle=x;
	  data[i].endAngle=x+v;
	  data[i].padAngle=w;
	  data[i].percent=data[i].value/(total||1);
	
      x+=v+w+(data[i].skipPad ? 0 : pad);	
    });
  }
  return chord;
}
  viz.defs = function(_){
	  var defs ={}, sel=_;
	  defs.sel =function(){ return sel;}
	  defs.lG= function(){ sel=sel.append("linearGradient"); return defs; }
	  defs.rG= function(){ sel=sel.append("radialGradient"); return defs; }
	  defs.stop= function(){ sel=sel.append("stop"); return defs; }
	  defs.filter= function(){ sel=sel.append("filter"); return defs; }
	  defs.feFlood= function(){ sel=sel.append("feFlood"); return defs; }
	  defs.feComposite= function(){ sel=sel.append("feComposite"); return defs; }
	  defs.feOffset= function(){ sel=sel.append("feOffset"); return defs; }
	  defs.feGaussianBlur= function(){ sel=sel.append("feGaussianBlur"); return defs; }
	  defs.result= function(_){ sel=sel.attr("result",_); return defs; }
	  defs.floodColor= function(_){ sel=sel.attr("flood-color",_); return defs; }
	  defs.floodOpacity= function(_){ sel=sel.attr("flood-opacity",_); return defs; }
	  defs.stdDeviation= function(_){ sel=sel.attr("stdDeviation",_); return defs; }
	  defs.operator= function(_){ sel=sel.attr("operator",_); return defs; }
	  defs.height= function(_){ sel=sel.attr("height",_); return defs; }
	  defs.width= function(_){ sel=sel.attr("width",_); return defs; }
	  defs.in= function(_){ sel=sel.attr("in",_); return defs; }
	  defs.in2= function(_){ sel=sel.attr("in2",_); return defs; }
	  defs.id= function(_){ sel=sel.attr("id",_); return defs; }
	  defs.fx= function(_){ sel=sel.attr("fx",_); return defs; }
	  defs.fy= function(_){ sel=sel.attr("fy",_); return defs; }
	  defs.dx= function(_){ sel=sel.attr("dx",_); return defs; }
	  defs.dy= function(_){ sel=sel.attr("dy",_); return defs; }
	  defs.x1= function(_){ sel=sel.attr("x1",_); return defs; }
	  defs.y1= function(_){ sel=sel.attr("y1",_); return defs; }
	  defs.x2= function(_){ sel=sel.attr("x2",_); return defs; }
	  defs.y2= function(_){ sel=sel.attr("y2",_); return defs; }
  	  defs.x= function(_){ sel=sel.attr("x",_); return defs; }
  	  defs.y= function(_){ sel=sel.attr("y",_); return defs; }
  	  defs.r= function(_){ sel=sel.attr("r",_); return defs; }
	  defs.spreadMethod= function(_){ sel=sel.attr("spreadMethod",_); return defs; }
  	  defs.gradientUnits= function(_){ sel=sel.attr("gradientUnits",_); return defs; }
	  defs.xlink= function(_){ sel=sel.attr("xlink:href",_); return defs; }
	  defs.offset= function(_){ sel=sel.attr("offset",_); return defs; }
	  defs.stopColor= function(_){ sel=sel.attr("stop-color",_); return defs; }
	  defs.path= function(){ sel=sel.append("path"); return defs; }
	  defs.d= function(_){ sel=sel.attr("d",_); return defs; }
	  return defs;
  }
  viz.legend = function(){
	var sel, data, rows, cols, width, paddingInner, paddingLabel, size, fill, rowScale, colScale, draw
		,onMouseOut, onMouseOver
	 
	function legend(_sel){ 
		sel=_sel;
		sel.select(".viz-legend").remove();		
		updateLocals();
				
		var cellHeight=rowScale.bandwidth(), cellWidth=colScale.bandwidth();
		
		var loc = d3.range(data.length).map(function(k){
			var x = k % cols, y = (k-x)/cols; 
			return {x:colScale(x), y:rowScale(y), width:size, height:cellHeight, key:data[k] };
		});

		var legendg = sel.append("g")
			.attr("class","viz-legend")
			.selectAll(".legend-item")
			.data(loc)
			.enter()
			.append("g")
			.attr("class","legend-item")
			.attr("transform",function(d){ return "translate("+d.x+","+d.y+")";})
			.on("mouseover",function(d){ return onMouseOver(d.key);})
			.on("mouseout",function(d){ return onMouseOut(d.key);})
			.each(draw);
	}
	legend.data = function(z){
		if(!arguments.length) return data;
		data = z;
		return legend;
	}
	legend.rowScale = function(_){
		return typeof rowScale !== "undefined" ? rowScale 
			: rowScale = d3.scaleBand()
				.domain(d3.range(legend.rows()))
				.range([0,legend.height()])
				.paddingInner(legend.paddingInner());
	}
	legend.colScale = function(_){
		return typeof colScale !== "undefined" ? colScale 
			: colScale = d3.scaleBand()
				.domain(d3.range(legend_cols()))
				.range([0,legend.width()]);
	}
	legend.rows = function(_){
		return arguments.length ? (rows = _, legend) :  typeof rows !== "undefined"  ? Math.min(rows, legend.data().length) : (rows = legend.data().length) ;
	}
	legend_cols = function(){
		return cols = Math.ceil(legend.data().length/(legend.rows() > 0 ? rows : 1)) ;
	}
	legend.width = function(_){
		return arguments.length ? (width = _, legend) :  typeof width !== "undefined" ? width : (width=100) ;
	}
	legend.height = function(_){
		var n=legend.rows();
		return arguments.length ? (height = _, legend) :  typeof height !== "undefined" ? height : (height=12*n+12*(n>0 ? n-1: 0)) ;
	}
	legend.paddingInner = function(_){
		return arguments.length ? (paddingInner = _, legend) :  typeof paddingInner !== "undefined" ? paddingInner : (paddingInner=rows > 1? .5 : 0) ;
	}
	legend.paddingLabel = function(_){
		return arguments.length ? (paddingLabel = _, legend) :  typeof paddingLabel !== "undefined" ? paddingLabel : (paddingLabel=6) ;
	}
	legend.size = function(_){
		return arguments.length ? (size = _, legend) :  typeof size !== "undefined" ? size : (size=12) ;
	}
	legend.onMouseOver = function(_){
		return arguments.length ? (onMouseOver = _, legend) :  typeof onMouseOver !== "undefined" ? onMouseOver : (onMouseOver=function(){}) ;
	}
	legend.onMouseOut = function(_){
		return arguments.length ? (onMouseOut = _, legend) :  typeof onMouseOut !== "undefined" ? onMouseOut : (onMouseOut=function(){}) ;
	}
	legend.fill = function(_){
		return arguments.length ? (fill = _, legend) :  typeof fill !== "undefined" ? fill : (fill=d3.scaleOrdinal().range(d3.schemeCategory10)) ;
	}
	legend.rect	= function(d){  
		d3.select(this)
			.append("rect")
			.attr("class",'legend-icon')
			.attr("height",viz_height)
			.attr("width",viz_width)
			.style('fill',function(d){ return fill(d.key); })
			;
			
		d3.select(this)
			.append("text")
			.attr("x",function(d){ return size+paddingLabel;})
			.attr("y",viz_height2)
			.attr("dy",6)
			.text(viz_key)
			;
	}  
	legend.circle = function(d){  
		d3.select(this)
			.append("circle")
			.attr("class",'legend-icon')
			.attr("r",9)
			.attr("cx",viz_width2)
			.attr('cy',viz_height2)
			.style('fill',function(d){ return fill(d.key); })
			;
			
		d3.select(this)
			.append("text")
			.attr("x",function(d){ return size+paddingLabel;})
			.attr("y",viz_height2)
			.attr("dy",6)
			.text(viz_key)
			;
	}
	legend.draw = function(_){
		return arguments.length ? (draw = _, legend) :  typeof draw !== "undefined" ? draw : (draw=legend.rect) ;
	}
	function updateLocals(){
		legend.rows();
		legend_cols();
		legend.width();
		legend.height();
		legend.paddingInner();
		legend.paddingLabel();
		legend.rowScale();
		legend.colScale();
		legend.size();
		legend.fill();
		legend.draw();
		legend.onMouseOver();
		legend.onMouseOut();
	}
		
	return legend;
  }	
  viz.uscs = function(){
	  
	  function uscs(_){ 
		g=_;
		viz_compute_uscounties();
		
        _.each(function() {
          var g = d3.select(this);
		  var state = uscs.state();
		  
		  var stateInfo = viz.maps.uscounties.stateByAbb[state];
		  var countyTopo = viz.maps.uscounties.objects.counties.geometries
						.filter(function(d) { return d.properties.SFP === stateInfo.FP; });
		  var countyData = topojson.feature(
						viz.maps.uscounties, {type: "GeometryCollection", geometries: countyTopo}
					).features;
					
		  var data = uscs.data();
		  if(data !== undefined){
			var countyMap = d3.map(data, uscs.countyFIPS());
		  
			countyData.forEach(function(d){ d.data= countyMap.get(""+d.properties.SFP+d.properties.CFP);});
		  }
		  
	      var projection =d3.geoMercator().rotate(stateInfo.r)
					.fitSize([uscs.width(), uscs.height()], topojson.merge(viz.maps.uscounties, countyTopo));
				
		  var path= d3.geoPath().projection(projection);
		  
	      var counties = g.append("g").attr("class","counties");
		  var countyBorder = g.append("path").attr("class","county-border");
		  var stateBorder = g.append("path").attr("class","state-border");	
		  var countyNames = g.append("g").attr("class","county-name");
		  
          stateBorder.datum(topojson.merge(viz.maps.uscounties, countyTopo)).attr("d", path);
		  
		  function CountyNamePos(d){return "translate(" + projection([d.properties.LON,d.properties.LAT]) + ")";}
	
          counties = counties.selectAll("path")
			  .data(countyData)
			  .enter().append("path")
			  .attr("d", path);
			  
		  var fill = uscs.fill();
	      if(fill !== undefined) counties.style("fill",function(d){ return fill(d.data); });
		  
          countyNames.selectAll("text")
			  .data(topojson.feature(viz.maps.uscounties, {type: "GeometryCollection", geometries: countyTopo}).features)
			  .enter().append("text")
			  .attr("transform", CountyNamePos)
			  .text(function(d){ return d.properties.CNM;});
  
		  countyBorder     
			  .datum(topojson.mesh(viz.maps.uscounties, {type: "GeometryCollection", geometries: countyTopo}, function(a,b){ return a!==b;}))
			  .attr("d",path);
		  
		});
	  }
	  uscs.data   = viz_assign(uscs);
	  uscs.state  = viz_assign(uscs);
	  uscs.width  = viz_assign_default(uscs, 960 );
	  uscs.height = viz_assign_default(uscs, 960 );
	  uscs.fill   = viz_assign_default(uscs, undefined );
	  uscs.countyFIPS   = viz_assign_default(uscs, function(d){return d[0]} );
	  
	  return uscs;
	}
  viz.calendar = function(){
    var data, date, value, fill, minYear, maxYear, cellWidth, cellHeight, yearPadding, valueLabel
	;
	var months = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"];
	var monthPosition = [3,7,11.5,15.5,20,24.5,28.5,33.5,37.5,42,46.5,50.5];
	var days =["S","M","T","W","T","F","S"];
	
	function cal(_){
		g=_;
		_.each(function(){
          var g = d3.select(this)
			,cellWidth=cal.cellWidth()
			,cellHeight=cal.cellHeight()
			,date = cal.date()
			,value = cal.value()
			,data = cal.data()
			,yearPadding = cal.yearPadding()
			,fill = cal.fill()
			,valueLabel =cal.valueLabel()
		  ;
		
		  var yearg = g.selectAll(".cal-year")
			  .data(d3.range(cal.minYear(), cal.maxYear()+1))
			  .enter().append("g").attr("class","cal-year")
			  .attr("transform", function(d,i){ return "translate(0," + ((yearPadding+ cellHeight*7)*i) + ")";});
			  
			yearg.append("text")
				.attr("transform", "translate(-6," + cellHeight * 3.5 + ")rotate(-90)")
				.attr("class", "cal-year-label")
				.text(function(d) { return d; });
			  
			yearg.filter(function(_,i){ return !i;})
				.selectAll(".cal-month-label").data(months).enter()
				.append("text")
				.attr("class", "cal-month-label")
				.attr("transform", function(d,i){ return "translate("+cellWidth*monthPosition[i]+",-6)"})
				.text(function(d) { return d; });
			  
			yearg.filter(function(_,i){ return !i;})
				.selectAll(".cal-day-label").data(days).enter()
				.append("text")
				.attr("class", "cal-day-label")
				.attr("transform", function(d,i){ return "translate("+(cellWidth*53+6)+","+(cellHeight*(i+0.5)+6)+")"})
				.text(function(d) { return d; });
				
			var rect = yearg.append("g")
				.selectAll(".cal-day")
				.data(function(d) { return d3.timeDays(new Date(d, 0, 1), new Date(d + 1, 0, 1)); })
				.enter().append("g").attr("class", "cal-day cal-day-empty")
				.attr("transform", function(d) { return "translate("+((d3.timeWeek.count(d3.timeYear(d), d)+.5)*cellWidth)+","+((d.getDay()+0.5)*cellHeight)+")"; })
				.datum(function(d){ return d})
				.append("rect")
				.attr("x", -.5*(cellWidth))
				.attr("y", -.5*(cellHeight) )
				.attr("width", cellWidth)
				.attr("height", cellHeight);	
				
			yearg.append("g")
				.selectAll(".cal-month")
				.data(function(d) { return d3.timeMonths(new Date(d, 0, 1), new Date(d + 1, 0, 1)); })
				.enter().append("path").attr("class", "cal-month")
				.attr("d", pathMonth);	
				
			var nestedData = d3.nest()
				.key(date)
				.rollup(function(d) { return value(d[0]); })
				.object(data);
				
			rect.filter(function(d) { return d in nestedData; })
				.attr("class", "cal-day cal-day-filled")
				.style("fill", function(d) { return fill(nestedData[d]); })
				.append("title")
				.text(function(d) { return d3.timeFormat("%Y-%m-%d")(d) + ": " + valueLabel(nestedData[d]); });	
				
			function pathMonth(t0) {
			  var t1 = new Date(t0.getFullYear(), t0.getMonth() + 1, 0),
				  d0 = t0.getDay(), w0 = d3.timeWeek.count(d3.timeYear(t0), t0),
				  d1 = t1.getDay(), w1 = d3.timeWeek.count(d3.timeYear(t1), t1);
			  return "M" + (w0 + 1) * cellWidth + "," + d0 * cellHeight
				  + "H" + w0 * cellWidth + "V" + 7 * cellHeight
				  + "H" + w1 * cellWidth + "V" + (d1 + 1) * cellHeight
				  + "H" + (w1 + 1) * cellWidth + "V" + 0
				  + "H" + (w0 + 1) * cellWidth + "Z";
			}				
		});
	}
	cal.data = function(_){
		if(!arguments.length) return data;
		data = _;
		return cal;
	}
	cal.cellWidth = function(_){
		if(!arguments.length) return typeof cellWidth !== "undefined" ? cellWidth : 17 ;
		cellWidth = _;
		return cal;
	}
	cal.cellHeight = function(_){
		if(!arguments.length) return typeof cellHeight !== "undefined" ? cellHeight : 17 ;
		cellHeight = _;
		return cal;
	}
	cal.date = function(_){ 
		if(!arguments.length) return typeof date !== "undefined" ? date : function(d){ return d.date; } ;
		date = _;
		return cal;		
	}
	cal.value = function(_){ 
		if(!arguments.length) return typeof value !== "undefined" ? value : function(d){ return d.value; } ;
		value = _;
		return cal;		
	}
	cal.valueLabel = function(_){ 
		if(!arguments.length) return typeof valueLabel !== "undefined" ? valueLabel : function(d){ return d;};
		valueLabel = _;
		return cal;		
	}
	cal.fill = function(_){ 
		if(!arguments.length) return typeof fill !== "undefined" ? fill : _fill();
		fill = _;
		return cal;		
	}
	cal.minYear = function(_){ 
		if(!arguments.length) return typeof minYear !== "undefined" ? minYear : _yearMin() ;
		minYear = _;
		return cal;		
	}	
	cal.maxYear = function(_){ 
		if(!arguments.length) return typeof maxYear !== "undefined" ? maxYear : _yearMax() ;
		maxYear = _;
		return cal;		
	}	
	cal.yearPadding = function(_){ 
		if(!arguments.length) return typeof yearPadding !== "undefined" ? yearPadding : 40 ;
		yearPadding = _;
		return cal;		
	}	
	function _fill(){
		var value = cal.value();
		
		return d3.scaleLinear()
			.domain(d3.extent(cal.data().map(function(d){ return value(d);})))
			.range(["#deebf7","#08306b"]);
	}
	function _yearMin(){
		var date = cal.date();
		return d3.min(cal.data().map(function(d){ return date(d).getFullYear();}));
	}
	function _yearMax(){
		var date = cal.date();
		return d3.max(cal.data().map(function(d){ return date(d).getFullYear();}));
	}
	return cal;
  }
  viz.pc = function(){
	var g, data, dimensions, dimensionLabelPadding, dimensionScale, valueScale
		,width, height, outerPadding, dimensionAxes, brushSize
	;
	var dragging = {}, background ,foreground, brushSelections = {}, path
	;
		
	function pc(_){
	  g=_;
      _.each(function() {
        var g = d3.select(this);
		updateVars();
		
		dimensions.forEach(function(d){ brushSelections[d]=null;});
		
		background = g.append("g")
			.attr("class", "background")
			.selectAll("path")
			.data(data)
			.enter().append("path")
			.attr("d", _path);
		
		foreground = g.append("g")
			.attr("class", "foreground")
			.selectAll("path")
			.data(data)
			.enter().append("path")
			.attr("d", _path);
			
		var dimg = g.selectAll(".dimension")
			.data(dimensions)
			.enter().append("g")
			.attr("class", "dimension")
			.attr("transform", function(d) { return "translate(" + dimensionScale(d) + ")"; })
			.call(d3.drag()
					.on("start", dragstart)
					.on("drag", drag)
					.on("end", dragend)
				);

		dimg.append("g")
			.attr("class", "axis")
			.each(function(d) { d3.select(this).call(dimensionAxes[d]); })
			.append("text")
			.style("text-anchor", "middle")
			.attr("y", dimensionLabelPadding)
			.text(function(d) { return d; });

		dimg.append("g")
			.attr("class", "brush")
			.each(function(d) {
				d3.select(this).call(
					valueScale[d].brush = d3.brushY()
						.extent([[-brushSize/2,0],[brushSize/2,height]])
						.on("end", brushend));
			});
			
		function _path(d) {
			return pc.path()(dimensions.map(function(p) { return [position(p), valueScale[p](d[p])]; }));
		}
		function position(d) {
		  var v = dragging[d];
		  return v == null ? dimensionScale(d) : v;
		}
		function dragstart(d) {
		  dragging[d] = dimensionScale(d);
		  background.attr("visibility", "hidden");
		}
		function drag(d) {
		  dragging[d] = Math.min(width, Math.max(0, d3.event.x));
		  
		  foreground.attr("d", _path);
		  
		  dimensions.sort(function(a, b) { return position(a) - position(b); });
		  
		  dimensionScale.domain(dimensions);
		  
		  dimg.attr("transform", function(d) { return "translate(" + position(d) + ")"; })
		}
		function dragend(d) {
		  delete dragging[d];
		  
		  d3.select(this).transition().duration(500).attr("transform", "translate(" + dimensionScale(d) + ")");
		  
		  foreground.transition().duration(500).attr("d", _path);
		  
		  background
			  .attr("d", _path)
			  .transition()
			  .delay(500)
			  .duration(0)
			  .attr("visibility", null);
		}
		function brushend(){
		  brushSelections[this.__data__] = d3.event.selection
			
		  var actives = dimensions.filter(function(p) { return brushSelections[p]; }),
			  extents = actives.map(function(p) { return brushSelections[p].map(valueScale[p].invert, valueScale[p]); });
			  
		  foreground.style("display", function(d) {
			return actives.every(function(p, i) {
			  return extents[i][1] <= d[p] && d[p] <= extents[i][0];
			}) ? null : "none";
		  });
		}			
	  });
	}
	
	pc.path = function(_){
	  if(!arguments.length) return typeof path !== 'undefined' ? path : (path= pc.bezier);
	  path = _;
	  return pc; 
	}
	pc.bezier = function(pl){
		var ret =[], b=pc.brushSize();
			
		pl.forEach(function(p,i){
			ret.push(i==0 ? "M"+(p[0]-b)+","+p[1]+"h"+2*b
				: "C"+((p[0]+pl[i-1][0])/2)+","+pl[i-1][1]+" "+((p[0]+pl[i-1][0])/2)+","+p[1]+","+p[0]+","+p[1]+"h"+b);
		});
		return ret.join("");
	}
	pc.lineSegments = function(pl){
		var b=pc.brushSize();					
		return pl.map(function(p,i){ return "M"+(p[0]-b)+","+p[1]+"h"+2*b; }).join("");
	}
	pc.data = function(_){ 
	  if(!arguments.length) return data;
	  data = _;
	  return pc; 
    }
    pc.dimensions = function(_){ 
	  if(!arguments.length) return typeof dimensions !== 'undefined' ? dimensions : (dimensions= d3.keys(data[0].sort(d3.ascending)));
	  dimensions = _;
	  return pc; 
    }
    pc.width = function(_){ 
	  if(!arguments.length) return typeof width !== 'undefined' ? width : (width= 900);
	  width = _;
	  return pc; 
    }
    pc.height = function(_){ 
	  if(!arguments.length) return typeof height !== 'undefined' ? height : (height= 600);
	  height = _;
	  return pc; 
    }
	pc.outerPadding = function(_){ 
	  if(!arguments.length) return typeof outerPadding !== 'undefined' ? outerPadding : (outerPadding= .1);
	  outerPadding = _;
	  return pc; 
    }
	pc.dimensionScale = function(_){ 
	  if(!arguments.length) return typeof dimensionScale !== 'undefined' 
		? dimensionScale 
		: (dimensionScale= d3.scalePoint().padding(pc.outerPadding()).range([0, pc.width()]).domain(dimensions));
		
	  dimensionScale = _;
	  return pc; 
    }
	pc.valueScale = function(_){ 
	  if(!arguments.length) {
		if(typeof valueScale !== 'undefined') return valueScale;
		
		var vScale = {}, _data = pc.data(), _height =pc.height();
		
		pc.dimensions().forEach(function(d){
			vScale[d] = d3.scaleLinear()
				.domain(d3.extent(_data, function(p) { return +p[d]; }))
				.range([_height, 0]);
			});
		return (valueScale = vScale);
	  }
	  valueScale = _;
	  return pc; 
    }
	pc.dimensionAxes = function(_){ 
	  if(!arguments.length) {
	    var vScale = pc.valueScale(), dAxes ={};
		
		if(typeof dimensionAxes !== 'undefined') {
			pc.dimensions().forEach(function(d){
				dAxes[d] = dimensionAxes[d].scale(vScale[d]);
			});
		}else{
			pc.dimensions().forEach(function(d){
				dAxes[d] = d3.axisLeft().scale(vScale[d]);
			});
		}
		return (dimensionAxes=dAxes);
	  }
	  dimensionAxes = _;
	  return pc; 
    }
	pc.brushSize = function(_){ 
	  if(!arguments.length) return typeof brushSize !== 'undefined' ? brushSize : (brushSize= 12);
	  brushSize = _;
	  return pc; 
    }
	pc.dimensionLabelPadding = function(_){ 
	  if(!arguments.length) return typeof dimensionLabelPadding !== 'undefined' ? dimensionLabelPadding : (dimensionLabelPadding= -9);
	  dimensionLabelPadding = _;
	  return pc; 
    }
	
	function updateVars(){
	  pc.dimensionScale();
	  pc.width();
	  pc.height();
	  pc.outerPadding();
	  pc.dimensionScale();
	  pc.valueScale();
	  pc.dimensionAxes();
	  pc.brushSize();
	  pc.dimensionLabelPadding();
	}

	return pc;
  }
  viz.area = function(){
    var sel, data, width, height, key, value0, value1, keyScale, valueScale, defined, curve, duration, ease
		, orient, orients = {'bottom':1, 'top':2, 'left':3, 'right':4 }
	
	function area(_sel){
		sel=_sel;
		sel.select(".viz-area").remove();
		
        var areag = sel.append("g").attr("class","viz-area")
			,points = area.points()
			;	

		var d3area = d3.area()
			.curve(area.curve())
			.defined(function(d){ return d.defined;})
			.x0(function(d){ return d.x0;})
			.x1(function(d){ return d.x1;})
			.y0(function(d){ return d.y0;})
			.y1(function(d){ return d.y1;})
		;

		areag.append("path")
			.datum(points)
			.attr("class", "area")
			.attr("d", d3area)
		;
	}
	area.data = function(z){
		if(!arguments.length) return data;
		data = z;
		return area;
	}
	area.path = function(){	
		return d3.area()
			.curve(area.curve())
			.defined(function(d){ return d.defined;})
			.x0(function(d){ return d.x0;})
			.x1(function(d){ return d.x1;})
			.y0(function(d){ return d.y0;})
			.y1(function(d){ return d.y1;})
			(area.points())
		;
	}
	area.transition = function(){
        var areag = sel.select(".viz-area")
			,points = area.points()
		;
		
		var d3area = d3.area()
			.curve(area.curve())
			.defined(function(d){ return d.defined;})
			.x0(function(d){ return d.x0;})
			.x1(function(d){ return d.x1;})
			.y0(function(d){ return d.y0;})
			.y1(function(d){ return d.y1;})
		;
		
		areag.select(".area")
			.datum(points)
			.transition()
			.duration(duration)
			.ease(ease)
			.attr("d", d3area)
		;
	}
	area.curve = function(_){
		return arguments.length ? (curve = _, area) :  typeof curve !== "undefined" ? curve : (curve=d3.curveLinear) ;
	}
	area.duration = function(_){
		return arguments.length ? (duration = _, area) :  typeof duration !== "undefined" ? duration : (duration=1000) ;
	}
	area.ease = function(_){
		return arguments.length ? (ease = _, area) :  typeof ease !== "undefined" ? ease : (ease=d3.easeLinear) ;
	}
	area.width = function(_){
		return !arguments.length ? (typeof width !== "undefined" ? width : (width=(area.orient(), orient =="bottom" || orient =="top" ? 880 : 420))) :(width = _, area);
	}
	area.height = function(_){
		return !arguments.length ? (typeof height !== "undefined" ? height : (height=( area.orient(), orient =="bottom" || orient =="top" ? 420 : 880))) :(height = _, area);
	}
	area.key = function(_){
		return arguments.length ? (key = typeof _ === "function" ? _ : function(){ return _ ;}, area) :  key || (key=function(d){ return d.key;}) ;
	}
	area.value1 = function(_){
		return arguments.length ? (value1 = typeof _ === "function" ? _ : function(){ return +_ ;}, area) :  value1 || (value1=function(d){ return d.value;});
	}
	area.value0 = function(_){
		return arguments.length ? (value0 = typeof _ === "function" ? _ : function(){ return +_ ;}, area) :  value0 || (value0=function(d){ return 0;});
	}
	area.keyScale = function(_){
		if(arguments.length) return (keyScale = _, area);
		var keyRange = {
			 "bottom": [0, area.width()] 
			,"top": [0, area.width()] 
			,"left": [0, area.height()] 
			,"right": [0, area.height()] 
			}[area.orient()];
				
		return keyScale || (keyScale=d3.scaleLinear()
			.domain(d3.extent(area.data().map(area.key())))
			.range(keyRange))
			;
	}
	area.valueScale = function(_){
		if(arguments.length) return (valueScale = _, area);
		
		var valueRange = {
			 "bottom": [area.height(), 0] 
			,"top": [0, area.height()]
			,"left": [0, area.width()] 
			,"right": [area.width(), 0] 
			}[area.orient()];

		return valueScale || (valueScale=d3.scaleLinear()
			.domain(yExtent())
			.range(valueRange));
	}
	area.defined = function(_){
		return arguments.length ? (defined = _, area) :  defined || (defined=function(){ return true;});
	}
	function yExtent(){
		var value0x = d3.extent(area.data().map(area.value0()));
		var value1x = d3.extent(area.data().map(area.value1()));
		return [d3.min([value0x[0], value1x[0]]),d3.max([value0x[1], value1x[1]])];
	}
	function updateLocals(){
		area.orient();
		area.key();
		area.value0();
		area.value1();
		area.keyScale();
		area.valueScale();
		area.defined();	
		area.duration();	
		area.curve();		
		area.ease();		
		area.width();		
		area.height();	
	}
	area.orient = function(_){
		return arguments.length ? (orient = _, area) :  orient || (orient='bottom');
	}
	area.points = function(){
		updateLocals();
		var x, y0, y1;
		
		if(orient== "bottom" || orient=="top"){
			x0=function(d,i){ return keyScale(key(d,i)) };
			x1=function(d,i){ return keyScale(key(d,i)) };
			y0=function(d,i){ return valueScale(value0(d,i)) };
			y1=function(d,i){ return valueScale(value1(d,i)) };
		}else{
			x0=function(d,i){ return valueScale(value0(d,i)); };
			x1=function(d,i){ return valueScale(value1(d,i)); };
			y0=function(d,i){ return keyScale(key(d,i)) };
			y1=function(d,i){ return keyScale(key(d,i)) };
		}
        return area.data().map(function(d,i){
				var ret={'defined':defined(d,i), 'data':d};
				ret.x0 = ret.defined ? x0(d,i): keyScale.range()[0];
				ret.x1 = ret.defined ? x1(d,i): keyScale.range()[0];
				ret.y0 = ret.defined ? y0(d,i): valueScale.range()[0];
				ret.y1 = ret.defined ? y1(d,i): valueScale.range()[1];
				return ret;
			});	
	}
	return area;
  }
  viz.line = function(){
    var sel, data, width, height, key, value, keyScale, valueScale, defined, curve, duration, ease
		,orient, orients = {'bottom':1, 'top':2, 'left':3, 'right':4 }
	;
	
	function line(_sel){
		sel=_sel;
		sel.select(".viz-line").remove();
		
        var lineg = sel.append("g").attr("class","viz-line")
			,points = line.points()
			
		var d3line = d3.line()
			.curve(line.curve())
			.defined(function(d){ return d.defined;})
			.x(function(d){ return d.x;})
			.y(function(d){ return d.y;})

		lineg.append("path")
			.datum(points)
			.attr("class", "line")
			.attr("d", d3line)
	}
	line.data = function(_){
		return !arguments.length ? data : (data = _, refresh=true, line);
	}
	line.transition = function(){
        var lineg = sel.select(".viz-line")
			,points = line.points()
		;
		
		var d3line = d3.line()
			.curve(line.curve())
			.defined(function(d){ return d.defined;})
			.x(function(d){ return d.x;})
			.y(function(d){ return d.y;})
		;
		
		lineg.select(".line")
			.datum(points)
			.transition()
			.duration(duration)
			.ease(ease)
			.attr("d", d3line)
		;		
	}
	line.curve = function(_){
		return !arguments.length ? (typeof curve !== "undefined" ? curve : (curve=d3.curveLinear)) :(curve = _, line);
	}
	line.duration = function(_){
		return !arguments.length ? (typeof duration !== "undefined" ? duration : (duration=1000)) :(duration = _, line);
	}
	line.ease = function(_){
		return !arguments.length ? (typeof ease !== "undefined" ? ease : (ease=d3.easeLinear)) :(ease = _, line);
	}
	line.width = function(_){
		return !arguments.length ? (typeof width !== "undefined" ? width : (width=(line.orient(), orient =="bottom"|| orient =="top" ? 880 : 420))) :(width = _, line);
	}
	line.height = function(_){
		return !arguments.length ? (typeof height !== "undefined" ? height : (height=( line.orient(), orient =="bottom"|| orient =="top" ? 420 : 880))) :(height = _, line);
	}
	line.key = function(_){
		return !arguments.length ? (typeof key !== "undefined" ? key : (key=function(d){ return d.key;})) :(key = _, line);
	}
	line.value = function(_){
		return !arguments.length ? (typeof value !== "undefined" ? value : (value=function(d){ return d.value;})) :(value = _, line);
	}
	line.keyScale = function(_){
		if(arguments.length) return (keyScale = _, line);
		
		var keyRange = {
			 "bottom": [0, line.width()] 
			,"top": [0, line.width()] 
			,"left": [0, line.height()] 
			,"right": [0, line.height()] 
			}[line.orient()];
		
		return keyScale || (keyScale=d3.scaleLinear()
			.domain(d3.extent(line.data().map(line.key())))
			.range(keyRange)) ;
	}
	line.valueScale = function(_){
		if(arguments.length) return (valueScale = _, line);
		
		var valueRange = {
			 "bottom": [line.height(), 0] 
			,"top": [0, line.height()]
			,"left": [0, line.width()] 
			,"right": [line.width(), 0] 
			}[line.orient()];
			
		return valueScale || (valueScale=d3.scaleLinear()
			.domain(d3.extent(line.data().map(line.value())))
			.range(valueRange)) ;
	}
	line.defined = function(_){
		return !arguments.length ? (typeof defined !== "undefined" ? value : (defined=function(){ return true;})) :(defined = _, line);
	}
	function updateLocals(){
		line.orient();
		line.key();
		line.value();
		line.keyScale();
		line.valueScale();
		line.defined();	
		line.duration();	
		line.curve();		
		line.ease();		
		line.width();		
		line.height();	
	}
	line.orient = function(_){
		return arguments.length ? (orient = _, line) :  orient || (orient='bottom');
	}
	line.points = function(){
		updateLocals();
		var x, y;
		
		if(orient=="bottom" || orient=="top"){
			x=function(d,i){ return keyScale(key(d,i)) };
			y=function(d,i){ return valueScale(value(d,i)) };
		}else{
			x=function(d,i){ return valueScale(value(d,i)) };
			y=function(d,i){ return keyScale(key(d,i)) };
		}		
        return line.data().map(function(d,i){
				var ret={'defined':defined(d,i), 'data':d};
				ret.x = x(d,i);
				ret.y = ret.defined ? y(d,i): valueScale.range()[0];
				return ret;
			});	
	}
	return line;
  }
  viz.point = function(){
    var sel, data, width, height, key, value, keyScale, valueScale
		,defined, curve, duration, ease, drawPoints, tree, points, refresh=true
		, orient, orients ={'bottom':1, 'top':2, 'left':3, 'right':4}
		;
	
	function point(_sel){
		sel=_sel;
		
		sel.select(".viz-point").remove();	

		 sel.append("g")
			.attr("class","viz-point")
			.selectAll(".point")
			.data(point.points()).enter()
			.append("g")
			.attr("class", "point")
			.attr("transform",function(d){ return "translate("+d.x+","+d.y+")";})
			.each(drawPoints)
		;
	}
	point.transition = function(){		
		sel.select(".viz-point").selectAll(".point")
			.data(point.points())
			.transition()
			.duration(duration)
			.ease(ease)
			.attr("transform",function(d){ return "translate("+d.x+","+d.y+")";})
		;		
	}
	point.data = function(_){
		return !arguments.length ? data : (data = _, refresh=true, point);
	}
	point.orient = function(_){
		return arguments.length ? (orient = orients[_], point) :  orient || (orient=1);
	}
	point.curve = function(_){
		return !arguments.length ? (typeof curve !== "undefined" ? curve : (curve=d3.curveLinear)) : (curve = _, point);
	}
	point.duration = function(_){
		return !arguments.length ? (typeof duration !== "undefined" ? duration : (duration=1000)) : (duration = _, point);
	}
	point.ease = function(_){
		return !arguments.length ? (typeof ease !== "undefined" ? ease : (ease=d3.easeLinear)) : (ease = _, point);
	}
	point.width = function(_){
		return !arguments.length ? (typeof width !== "undefined" ? width : (refresh=true, width=880)) : (width = _, refresh=true, point);
	}
	point.height = function(_){
		return !arguments.length ? (typeof height !== "undefined" ? height : (refresh=true, height=420)) : (height = _, refresh=true, point);
	}
	point.key = function(_){
		return !arguments.length ? (key || (refresh=true, key=function(d){ return d.key;})) : (key = _, refresh=true, point);
	}
	point.value = function(_){
		return !arguments.length ? (value || (refresh=true, value=function(d){ return d.value;})) : (value = _, refresh=true, point);
	}
	function point_xRange(){
		
	}
	point.keyScale = function(z){
		if(!arguments.length) {
			var keyRange = {
				 1: [0, point.width()] 
				,2: [0, point.width()] 
				,3: [0, point.height()] 
				,4: [0, point.height()] 
				}[orient];
			
			return keyScale || (refresh=true, keyScale=d3.scaleLinear()
				.domain(d3.extent(point.data().map(point.key())))
				.range(keyRange))
				;
		}
		
		return (keyScale = z, refresh=true, point);
	}
	point.valueScale = function(_){		
		if(!arguments.length) {
			var valueRange = {
				 1: [point.height(), 0] 
				,2: [0, point.height()]
				,3: [0, point.width()] 
				,4: [point.width(), 0] 
				}[orient];
				
			return valueScale || (refresh=true, valueScale=d3.scaleLinear()
				.domain(d3.extent(point.data().map(point.value())))
				.range(valueRange)) 
				;
		}
		return (valueScale = _, refresh=true, point);
	}
	point.defined = function(z){
		if(!arguments.length) return defined || (refresh=true, defined=function(){ return true;}) ;
		return (defined = z, refresh=true, point);
	}
	point.drawPoints = function(_){
		return arguments.length ? (drawPoints = _, point) 
			: drawPoints || (drawPoints=function(){ 					
					d3.select(this).append("circle").attr("r",6);
				}) ;
	}
	function updateLocals(){
		if(!refresh) return; 
		point.orient();
		point.key();
		point.value();
		point.keyScale();
		point.valueScale();
		point.defined();	
		point.duration();	
		point.curve();		
		point.ease();		
		point.width();		
		point.height();	
		point.drawPoints();		
		
        points = point.data().map(function(d,i){
				var ret={'defined':defined(d,i), 'data':d, 'selected':false};
				ret.x = keyScale(key(d,i));
				ret.y = ret.defined ? valueScale(value(d,i)): valueScale.range()[0];
				return ret;
			});	
		
		tree = d3.quadtree()
			.x(function(d){ return d.x})
			.y(function(d){ return d.y})
			.addAll(points)
			.extent([[keyScale.range()[0], valueScale.range()[0]], [keyScale.range()[1]+1, valueScale.range()[1]+1]])
			;	
		refresh=false;
	}
	point.points = function(){
		updateLocals();		
        return points;
	}
	point.tree = function(){
		updateLocals();		
        return tree;
	}
	point.filterRect = function(r) {
		points.forEach(function(d){ d.selected=false;});
		
		if(!r) return point;
		
		var x0=r[0][0], y0=r[0][1], x3=r[1][0], y3=r[1][1];
		
		tree.visit(function(node, x1, y1, x2, y2) {
			var p = node.data;
			
			if (p) p.selected = (p.x >= x0) && (p.x < x3) && (p.y >= y0) && (p.y < y3);
			
			return x1 >= x3 || y1 >= y3 || x2 < x0 || y2 < y0;
		});
		return point;
	}
	return point;
  }
  viz.bar = function(){
    var sel, data, width, height, key, value0, value1, keyScale, valueScale, defined, curve, duration, ease
		, align, paddingInner, paddingOuter, orient, orients = {'bottom':1, 'top':2, 'left':3, 'right':4 }
	;
	
	function bar(_sel){
		sel=_sel;
		sel.select(".viz-bar").remove();
		
        var barg = sel.append("g").attr("class","viz-bar")

		barg.selectAll(".bar")
			.data(bar.bars())
			.enter()
			.append("rect")
			.attr("class", "bar")
			.attr("x", function(d){ return d.x;})
			.attr("y", function(d){ return d.y;})
			.attr("width", function(d){ return d.width;})
			.attr("height", function(d){ return d.height;})
	}
	bar.data = function(_){
		return !arguments.length ? data : (data = _, bar);
	}
	bar.transition = function(){
		
		sel.select(".viz-bar")
			.selectAll(".bar")
			.data(bar.bars())
			.transition()
			.duration(duration)
			.ease(ease)
			.attr("x", function(d){ return d.x;})
			.attr("y", function(d){ return d.y;})
			.attr("width", function(d){ return d.width;})
			.attr("height", function(d){ return d.height;})
	}
	bar.curve = function(_){
		return arguments.length ? (curve = _, bar) :  typeof curve !== "undefined" ? curve : (curve=d3.curveLinear) ;
	}
	bar.duration = function(_){
		return arguments.length ? (duration = _, bar) :  typeof duration !== "undefined" ? duration : (duration=1000) ;
	}
	bar.ease = function(_){
		return arguments.length ? (ease = _, bar) :  typeof ease !== "undefined" ? ease : (ease=d3.easeLinear) ;
	}
	bar.width = function(_){
		return !arguments.length ? (typeof width !== "undefined" ? width : (width=(bar.orient(),  880))) :(width = _, bar);
	}
	bar.height = function(_){
		return !arguments.length ? (typeof height !== "undefined" ? height : (height=( bar.orient(), 420))) :(height = _, bar);
	}
	bar.key = function(_){
		return arguments.length ? (key = typeof _ === "function" ? _ : function(){ return _ ;}, bar) :  key || (key=function(d){ return d.key;}) ;
	}
	bar.value1 = function(_){
		return arguments.length ? (value1 = typeof _ === "function" ? _ : function(){ return +_ ;}, bar) :  value1 || (value1=function(d){ return d.value;});
	}
	bar.value0 = function(_){
		return arguments.length ? (value0 = typeof _ === "function" ? _ : function(){ return +_ ;}, bar) :  value0 || (value0=function(d){ return 0;});
	}
	bar.paddingInner = function(_){
		return arguments.length ? (paddingInner = _, bar) :  typeof paddingInner !== "undefined" ? paddingInner : (paddingInner=0.1) ;
	}
	bar.paddingOuter = function(_){
		return arguments.length ? (paddingOuter = _, bar) :  typeof paddingOuter !== "undefined" ? paddingOuter : (paddingOuter=0.1) ;
	}
	bar.align = function(_){
		return arguments.length ? (align = _, bar) :  typeof align !== "undefined" ? align : (align=0.5) ;
	}
	bar.keyScale = function(_){
		if(arguments.length) return (keyScale = _, bar);
		var keyRange = {
			 "bottom": [0, bar.width()] 
			,"top": [0, bar.width()] 
			,"left": [0, bar.height()] 
			,"right": [0, bar.height()] 
			}[bar.orient()];
			
		return keyScale || (keyScale=d3.scaleBand()
			.domain(bar.data().map(bar.key()))
			.range(keyRange)
			.paddingInner(bar.paddingInner())
			.paddingOuter(bar.paddingOuter()))
			.align(bar.align())
	}
	bar.valueScale = function(_){
		if(arguments.length) return (valueScale = _, bar);
		
		var valueRange = {
			 "bottom": [bar.height(), 0] 
			,"top": [0, bar.height()]
			,"left": [0, bar.width()] 
			,"right": [bar.width(), 0] 
			}[bar.orient()];

		return valueScale || (valueScale=d3.scaleLinear()
			.domain(yExtent())
			.range(valueRange));
	}
	bar.defined = function(_){
		return arguments.length ? (defined = _, bar) :  defined || (defined=function(){ return true;});
	}
	function yExtent(){
		var value0x = d3.extent(bar.data().map(bar.value0()));
		var value1x = d3.extent(bar.data().map(bar.value1()));
		return [d3.min([value0x[0], value1x[0]]),d3.max([value0x[1], value1x[1]])];
	}
	function updateLocals(){
		bar.orient();
		bar.key();
		bar.value0();
		bar.value1();
		bar.keyScale();
		bar.valueScale();
		bar.defined();	
		bar.duration();	
		bar.curve();		
		bar.ease();		
		bar.width();		
		bar.height();	
	}
	bar.orient = function(_){
		return arguments.length ? (orient = _, bar) :  orient || (orient='bottom');
	}
	bar.bars = function(){
		updateLocals();
		var _x, _y, _width, _height;
		
		if(orient== "bottom"){
			_x=function(d,i){ return keyScale(key(d,i)) };
			_y=function(d,i){ return valueScale(value1(d,i)) };
			_height=function(d,i){ return valueScale(value0(d,i)) - valueScale(value1(d,i)); };
			_width=function(d,i){ return keyScale.bandwidth(); };
		}else if(orient== "top"){
			_x=function(d,i){ return keyScale(key(d,i)) };
			_y=function(d,i){ return valueScale(value0(d,i)) };
			_height=function(d,i){ return valueScale(value1(d,i)) - valueScale(value0(d,i)); };
			_width=function(d,i){ return keyScale.bandwidth(); };
		}else if(orient== "left"){
			_x=function(d,i){ return valueScale(value0(d,i)) };
			_y=function(d,i){ return keyScale(key(d,i)) };
			_height=function(d,i){ return keyScale.bandwidth(); };
			_width=function(d,i){ return valueScale(value1(d,i)) - valueScale(value0(d,i)); };
		}else {//orient== "right"
			_x=function(d,i){ return valueScale(value1(d,i)) };
			_y=function(d,i){ return keyScale(key(d,i)) };
			_height=function(d,i){ return keyScale.bandwidth(); };
			_width=function(d,i){ return valueScale(value0(d,i)) - valueScale(value1(d,i)); };
		}
		
        return bar.data().map(function(d,i){
				var ret={'defined':defined(d,i), 'data':d};
				ret.x = ret.defined ? _x(d,i): keyScale.range()[0];
				ret.y = ret.defined ? _y(d,i): keyScale.range()[0];
				ret.width = ret.defined ? _width(d,i): valueScale.range()[0];
				ret.height = ret.defined ? _height(d,i): valueScale.range()[1];
				return ret;
			});	
	}
	return bar;
  }
  viz.pie3d = function(){
    var g, data, innerRadius, outerRadius, value, fill, startAngle, endAngle
		,eccentricity, height, label
	
	function pie3d(_g){
	  g=_g;
	  
	  var pielayout = d3.pie()
			.value(pie3d.value())
			.startAngle(pie3d.startAngle())
			.endAngle(tau+pie3d.startAngle())
			(pie3d.data())
		
		var gpie3d = g.append("g").attr("class", "viz-pie3d")		 
			,_fill = pie3d.fill()
		
		gpie3d.selectAll(".innerSlice")
			.data(pielayout)
			.enter()
			.append("path")
			.attr("class", "innerSlice")
			.style("fill", function(d) { return d3.hsl(_fill(d.data)).darker(0.7); })
			.attr("d",pieInner)
			.each(function(d){this._current=d;});
				
		gpie3d.selectAll(".topSlice")
			.data(pielayout)
			.enter()
			.append("path")
			.attr("class", "topSlice")
			.style("fill", function(d){ return _fill(d.data);})
			.style("stroke", function(d) { return _fill(d.data); })
			.attr("d",pieTop)
			.each(function(d){this._current=d;});
			
		gpie3d.selectAll(".outerSlice")
			.data(pielayout)
			.enter()
			.append("path")
			.attr("class", "outerSlice")
			.style("fill", function(d) { return d3.hsl(_fill(d.data)).darker(0.7); })
			.attr("d",pieOuter)
			.each(function(d){this._current=d;});
			
		var lx = (pie3d.outerRadius()+pie3d.innerRadius())/2
			,ly = pie3d.eccentricity()*lx
			
		gpie3d.selectAll(".label")
			.data(pielayout).enter().append("text").attr("class", "label")
			.attr("x",function(d){ return lx*Math.cos(0.5*(d.startAngle+d.endAngle));})
			.attr("y",function(d){ return ly*Math.sin(0.5*(d.startAngle+d.endAngle));})
			.attr("dy",6)
			.text(function(d){return pie3d.label()(d); })
			.each(function(d){this._current=d;});	
			
		function pieInner(d){
			var startAngle = (d.startAngle < pi ? pi : d.startAngle);
			var endAngle = (d.endAngle < pi ? pi : d.endAngle);
			
			var ecc = pie3d.eccentricity()
				,rx = pie3d.innerRadius()
				,ry = ecc*rx
				,h  = pie3d.height()
				,sx = rx*Math.cos(startAngle)
				,sy = ry*Math.sin(startAngle)
				,ex = rx*Math.cos(endAngle)
				,ey = ry*Math.sin(endAngle);

				return ["M",sx, sy,"A",rx,ry,"0 0 1",ex,ey, "L",ex,h+ey,"A",rx, ry,"0 0 0",sx,h+sy,"z"].join(" ");
		}
		function pieTop(d){
			if(d.endAngle - d.startAngle == 0 ) return "M 0 0";
			var ecc = pie3d.eccentricity()
				,rx = pie3d.outerRadius()
				,ry = ecc*rx
				,ir = pie3d.innerRadius()/rx
				,sx = rx*Math.cos(d.startAngle)
				,sy = ry*Math.sin(d.startAngle)
				,ex = rx*Math.cos(d.endAngle)
				,ey = ry*Math.sin(d.endAngle);
				
			return ["M",sx,sy,"A",rx,ry,"0",(d.endAngle-d.startAngle > pi ? 1: 0),"1",ex,ey,"L",ir*ex,ir*ey,
			"A",ir*rx,ir*ry,"0",(d.endAngle-d.startAngle > pi ? 1: 0), "0",ir*sx,ir*sy,"z"].join(" ");
		}
		function pieOuter(d){
			var startAngle = (d.startAngle > pi ? pi : d.startAngle);
			var endAngle = (d.endAngle > pi ? pi : d.endAngle);
			
			var ecc = pie3d.eccentricity()
				,rx = pie3d.outerRadius()
				,ry = ecc*rx
				,ir = pie3d.innerRadius()/rx
				,h  = pie3d.height()
				,sx = rx*Math.cos(startAngle)
				,sy = ry*Math.sin(startAngle)
				,ex = rx*Math.cos(endAngle)
				,ey = ry*Math.sin(endAngle);
				
				return ["M",sx,h+sy,"A",rx,ry,"0 0 1",ex,h+ey,"L",ex,ey,"A",rx,ry,"0 0 0",sx,sy,"z"].join(" ");
		}

	}
    pie3d.data = function(x){ 
	  if(!arguments.length) return data;
	  data = x;
	  return pie3d; 
    }
    pie3d.value = function(x){ 
	  if(!arguments.length) return typeof value !== "undefined" ? value : (value=function(d){ return d; } );
	  value = (typeof x== 'function' ? x : function(d){ return +x;});
	  return pie3d; 
    }
    pie3d.label = function(x){ 
	  if(!arguments.length) return typeof label !== "undefined" ? label : (label=function(d){ return '';});
	  label = (typeof x== 'function' ? x : function(d){ return x;});
	  return pie3d; 
    }
    pie3d.fill = function(x){ 
	  if(!arguments.length) return typeof fill !== "undefined" ? fill : (fill=function(d){ return "black"; } );
	  fill = (typeof x== 'function' ? x : function(d){ return x;});
	  return pie3d; 
    }
    pie3d.height = function(x){ 
	  if(!arguments.length) return typeof height !== "undefined" ? height : (height=30 );
	  height = x;
	  return pie3d; 
    }
    pie3d.innerRadius = function(x){ 
	  if(!arguments.length) return typeof innerRadius !== "undefined" ? innerRadius : (innerRadius=0);
	  innerRadius = x;
	  return pie3d; 
    }
    pie3d.outerRadius = function(x){ 
	  if(!arguments.length) return typeof outerRadius !== "undefined" ? outerRadius : (outerRadius=200);
	  outerRadius = x;
	  return pie3d; 
    }
    pie3d.eccentricity = function(x){ 
	  if(!arguments.length) return typeof eccentricity !== "undefined" ? eccentricity : (eccentricity=.8);
	  eccentricity = x;
	  return pie3d; 
    }
    pie3d.startAngle = function(x){ 
	  if(!arguments.length) return typeof startAngle !== "undefined" ? startAngle : (startAngle=0);
	  startAngle = x;
	  return pie3d; 
    }
	
    return pie3d;
  }
  viz.pie = function(){
    var g, data, innerRadius, outerRadius, cornerRadius, startAngle
	  , endAngle, padAngle, value, fill, sort, label, ease
	  , duration, folded=false
	
	function pie(_){
	  g=_;
      _.each(function() {		
		d3.select(this).select(".viz-pie").remove();
		
		var sel = d3.select(this).append("g").attr("class","viz-pie")
		
		var _fill = pie.fill()
		  ,_label = pie.label()
		  ,_data = pie.d3pie()(pie.data())
		  ,_arc = pie.arc()
		  
		if(folded) _arc = _arc.startAngle(pie.startAngle()).endAngle(pie.startAngle())
		
		sel.append("g")
		  .attr("class", "arcs")
		  .selectAll(".arc").data(_data).enter().append("path").attr("class", "arc")
  	      .attr("d", _arc)
          .style("fill", function(d){ return _fill(d.data);})
		  
        sel.append("g")
		  .attr('class','labels')
		  .selectAll(".label").data(_data).enter().append("text").attr('class','label')
          .attr("transform", function(d) { return "translate(" + _arc.centroid(d) + ")"; })
		  .attr("dy","0.35em")
          .text(function(d){ return folded ? "" : _label(d.data); })
	  })
	}
    pie.fold = function(){
	  if(folded) return; // if it is already folded, do nothing;
	  
      g.each(function() {
        var sel = d3.select(this).select(".viz-pie");
		
		var _duration = pie.duration()
		  ,_arc = pie.arc()
		  ,_label = pie.label()
		  ,_ease = pie.ease()
		
		sel.select(".arcs")
		  .selectAll(".arc")
		  .transition()
		  .duration(_duration)
  	      .attrTween("d", arcTween)
		
		sel.select(".labels")
		  .selectAll(".label")
          .text(function(d){ return "";})
	
		function arcTween(d) {			  
            var _startAngle = pie.startAngle()(d.data);
		    var interpolateStart = d3.interpolate(d.startAngle, _startAngle);
		    var interpolateEnd = d3.interpolate(d.endAngle, _startAngle);
			return function(t) {
			  d.startAngle = interpolateStart(_ease(t));
			  d.endAngle = interpolateEnd(_ease(t));
			  return _arc(d);
			};
		}		
	  });
	  folded=true;
	}
    pie.unfold = function(){
	  if(!folded) return; // if it is already folded, do nothing;
	  
      g.each(function() {
        var sel = d3.select(this).select(".viz-pie");
		
		var _duration = pie.duration()
		  ,_arc = pie.arc()
		  ,_label = pie.label()
		  ,_data = pie.d3pie()(pie.data())
		  ,_ease = pie.ease()
		
		sel.select(".arcs")
		  .selectAll(".arc")
		  .data(_data)
		  .transition()
		  .duration(_duration)
  	      .attrTween("d", arcTween)
		
		sel.select(".labels")
		  .selectAll(".label")
		  .data(_data)
          .attr("transform", function(d) { return "translate(" + _arc.centroid(d) + ")"; })
		  .transition()
		  .delay(_duration)
          .text(function(d){ return _label(d.data);})

		function arcTween(d) {
            var _startAngle = pie.startAngle()(d.data);
		    var interpolateStart = d3.interpolate(_startAngle, d.startAngle);
		    var interpolateEnd = d3.interpolate(_startAngle, d.endAngle);
			return function(t) {
			  d.startAngle = interpolateStart(_ease(t));
			  d.endAngle = interpolateEnd(_ease(t));
			  return _arc(d);
			};
		}
	  });
	  folded=false;
	}
    pie.createFolded = function(){ 
	  folded = true;
	  return pie; 
    }
    pie.data = function(x){ 
	  if(!arguments.length) return data;
	  data = x;
	  return pie; 
    }
    pie.value = function(x){ 
	  if(!arguments.length) return typeof value !== "undefined" ? value : function(d){ return d; } ;
	  value = (typeof x== 'function' ? x : function(d){ return +x;});
	  return pie; 
    }
    pie.innerRadius = function(x){ 
	  if(!arguments.length) return typeof innerRadius !== "undefined" ? innerRadius : function(d){ return 0; } ;
	  innerRadius = (typeof x== 'function' ? x : function(d){ return +x;});
	  return pie; 
    }
    pie.outerRadius = function(x){ 
	  if(!arguments.length) return typeof outerRadius !== "undefined" ? outerRadius : function(d){ return 200; } ;
	  outerRadius = (typeof x== 'function' ? x : function(d){ return +x;});
	  return pie; 
    }
    pie.cornerRadius = function(x){ 
	  if(!arguments.length) return typeof cornerRadius !== "undefined" ? cornerRadius : function(d){ return 0; } ;
	  cornerRadius = (typeof x== 'function' ? x : function(d){ return +x;});
	  return pie; 
    }
    pie.startAngle = function(x){ 
	  if(!arguments.length) return typeof startAngle !== "undefined" ? startAngle : function(d){ return 0; } ;
	  startAngle = (typeof x== 'function' ? x : function(d){ return +x;});
	  return pie; 
    }
    pie.endAngle = function(x){ 
	  if(!arguments.length) return typeof endAngle !== "undefined" ? endAngle : function(d){ return pie.startAngle()(d) + 2*Math.PI; } ;
	  endAngle = (typeof x== 'function' ? x : function(d){ return +x;});
	  return pie; 
    }
    pie.padAngle = function(x){ 
	  if(!arguments.length) return typeof padAngle !== "undefined" ? padAngle : function(d){ return 0; } ;
	  padAngle = (typeof x== 'function' ? x : function(d){ return +x;});
	  return pie; 
    }
    pie.fill = function(x){ 
	return arguments.length ? (fill = x, pie) 
	: typeof fill !== "undefined" ? fill : (fill=viz_schemeCategory10()) ;
/*	  if(!arguments.length) return typeof fill !== "undefined" ? fill : function(d){ return '#000'; } ;
	  fill = (typeof x== 'function' ? x : function(d){ return x;});
	  return pie; */
    }
    pie.sort = function(x){ 
	  if(!arguments.length) return typeof sort !== "undefined" ? sort : null ;
	  sort = x;
	  return pie; 
    }
    pie.label = function(x){ 
	  if(!arguments.length) return typeof label !== "undefined" ? label : function(d){ return '';};
	  label = x;
	  return pie; 
    }
	pie.duration = function(x){ 
	  if(!arguments.length) return typeof duration !== "undefined" ? duration : 500 ;
	  duration = x;
	  return pie; 
    }
	pie.ease = function(x){ 
	  if(!arguments.length) return typeof ease !== "undefined" ? ease : d3.easeCubicInOut ;
	  ease = x;
	  return pie; 
    }
	pie.arc = function(){
	  return d3.arc()
        .outerRadius(pie.outerRadius())
        .innerRadius(pie.innerRadius())
        .cornerRadius(pie.cornerRadius())
	}
	pie.d3pie = function(){
      return d3.pie()
        .sort(pie.sort())
	    .startAngle(pie.startAngle())
	    .endAngle(pie.endAngle())
	    .padAngle(pie.padAngle())
        .value(pie.value())
	}
    return pie;
  }
  viz.form={}
  viz.form.select = function(){
	var sel, data, value, text, onChange
	  
	function select(_){
	  sel=_;
	  
	  _.selectAll("option").remove()
	  
	  _.selectAll("option")
	  	.data(select.data())
	  	.enter()
	  	.append("option")
	  	.property("value",select.value())
	  	.text(select.text())
		
	  _.on("change",select.onChange())
	}
	select.data =  function(_){
	  return !arguments.length ? data : (data = _, select);
	}
	select.value = function(_){
		return arguments.length ? (value = _, select) :  value || (value=function(d){ return d});
	}
	select.text = function(_){
		return arguments.length ? (text = _, select) :  text || (text=function(d){ return d});
	}
	select.onChange = function(_){
		return arguments.length ? (onChange = function(){ _(select.get())}, select) :  onChange || (onChange=function(){ });
	}
	select.set = function(_){
		return (sel.property("value",_), select);
	}
	select.get = function(){
		return sel.property("value");
	}
	select.update = function(_){
		data = _;
		sel.selectAll("option").remove()
		
		sel.selectAll("option")
			.data(_)
			.enter()
			.append("option")
			.property("value",select.value())
			.text(select.text())
			
		sel.on("change",select.onChange())
			
		return select;
	}
    return select;
  }  
  viz.form.checkList = function(){
    var data, sel, buttonLabel, optionLabel, onClick, dataValue, defaultCheck
		,selectedIndices, onHidden, allNoneButton
	;
	
	function checkList(s){
      sel = s;
	  selectedIndices=checkList.data().map(checkList.defaultCheck());
	  
	  sel.classed("button-group", true)
		.classed("dropdown", true)
		.append("button")
		.attr("class","btn btn-secondary  dropdown-toggle")
		.attr("data-toggle","dropdown")
		.text(checkList.buttonLabel())
		;
		
	  $(sel.node()).on("hidden.bs.dropdown", checkList.onHidden());
	
	  var ul = sel.append("ul")
		.attr("class","dropdown-menu")
		;
	
	  if(checkList.allNoneButton()){
		var buttons = ul.append("li").attr("class","buttons").append("div").attr("class","row");
		
		buttons.selectAll("button").data(["All","None"]).enter().append("button")
			.attr("type","button").attr("class","btn btn-secondary btn-sm col-3")
			.text(function(d){ return d;})
			.on("click",allNoneClicked);
	  }
	  
	  ul.selectAll(".list").data(checkList.data()).enter().append("li").attr("class","list")
		.append("a")
		.on("click",_onClick)
		.attr("href","#")
		.attr("class","dropdown-item")
		.attr("data-value",checkList.dataValue())
		.attr("tabIndex","-1")
		.each(function(d){
			d3.select(this)
				.append("input")
				.attr("type","checkbox")
				.property("checked",checkList.defaultCheck())
		})
		.append("span")
		.text(checkList.optionLabel());	
	}
			
	checkList.data = function(_){
		if(!arguments.length) return data;
		data = _;
		return checkList;
	}
	checkList.buttonLabel = function(_){
		if(!arguments.length) return typeof buttonLabel !== "undefined" ? buttonLabel : (buttonLabel="Options") ;
		buttonLabel = _;
		return checkList;
	}
	checkList.allNoneButton = function(_){
		if(!arguments.length) return typeof allNoneButton !== "undefined" ? allNoneButton : (allNoneButton=true) ;
		allNoneButton = _;
		return checkList;
	}
	checkList.optionLabel = function(_){
		if(!arguments.length) return typeof optionLabel !== "undefined" ? optionLabel : (optionLabel=function(d){ return d}) ;
		optionLabel = _;
		return checkList;
	}
	checkList.onClick = function(_){
		if(!arguments.length) return typeof onClick !== "undefined" ? onClick : (onClick=function(d){ }) ;
		onClick = _;
		return checkList;
	}
	checkList.onHidden = function(_){
		if(!arguments.length) return typeof onHidden !== "undefined" ? onHidden : (onHidden=function(d){ }) ;
		onHidden = _;
		return checkList;
	}
	checkList.dataValue = function(_){
		if(!arguments.length) return typeof dataValue !== "undefined" ? dataValue : (dataValue=function(d){ return d}) ;
		dataValue = _;
		return checkList;
	}
	checkList.defaultCheck = function(_){
		if(!arguments.length) return typeof defaultCheck !== "undefined" ? defaultCheck : (defaultCheck=function(d){ return true}) ;
		defaultCheck = _;
		return checkList;
	}
	checkList.selectionList = function(){
		return checkList.data().map(function(_,i){ return selectedIndices[i];});
	}	
	function _onClick(d,i){
		var idx=checkList.data().indexOf( d );
		d3.select(this).select("input").property("checked",(selectedIndices[idx]=!selectedIndices[idx]))
		
		checkList.onClick()(d,i);
		
		d3.event.stopPropagation();
	}
	function allNoneClicked(d){
		if(d=="All") selectedIndices=checkList.data().map(function(d){ return true;});
		else selectedIndices=checkList.data().map(function(d){ return false;});
		
		sel.selectAll(".list")
			.select("input")
			.property("checked",function(d,i){ return selectedIndices[i];});
			
		d3.event.stopPropagation();	  
	}
	return checkList;
  }
  viz.table = function(){
	var body, header, footer;
	
	function table(_){
		g=_;
		_.each(function(){
		  var g = d3.select(this);
		
		  g.append("thead").append("tr")
			.selectAll("th").data(table.header()).enter().append("th")
			.text(function(d){ return d;})
		
		  g.append("tbody")
			.selectAll("tr").data(table.body()).enter().append("tr")
			.selectAll("td").data(function(d){ return d;}).enter().append("td")
			.text(function(d){ return d;})
		
		  g.append("tfoot").append("tr")
			.selectAll("th").data(table.footer()).enter().append("th")
			.text(function(d){ return d;})
		});
	}
	table.body = function(_){
		if(!arguments.length) return tbody;
		tbody = _;
		return table;
	}	
	table.header = function(_){
		if(!arguments.length) return typeof header !== "undefined" ? header : [];
		header = _;
		return table;
	}	
	table.footer = function(_){
		if(!arguments.length) return typeof footer !== "undefined" ? footer : [];
		footer = _;
		return table;
	}	
	return table;
  }
  viz.navTabs = function(){ // fix set function
	var sel, data, id, text
	  
	function navTabs(_){
	  sel=_;
	  
	  sel.selectAll(".viz-navTabs").remove()
	  
	  sel.append("ul")
		.attr("class","nav nav-tabs viz-navTabs")
		.attr("role","tablist")
		.selectAll(".nav-item")
		.data(navTabs.data())
		.enter().append("li")
		.attr("class","nav-item")
		.append("a")
		.attr("class","nav-link")
		.classed("active",function(d,i){ return i==0 })
		.attr("data-toggle","tab")
		.attr("href",navTabs.id())
		.attr("role","tab")
		.text(navTabs.text())	  
	}
	navTabs.data =  function(_){
	  return !arguments.length ? data : (data = _, navTabs);
	}
	navTabs.id = function(_){
		return arguments.length ? (id = _, navTabs) :  id || (id=function(d){ return d.id});
	}
	navTabs.text = function(_){
		return arguments.length ? (text = _, navTabs) :  text || (text=function(d){ return d.text});
	}
	navTabs.set = function(_){
		sel.selectAll(".nav-item").each(function(d){			
			d3.select(this).select(".nav-link")
				.classed("active", navTabs.text()(d)==_)
		})
		
		return  navTabs;
	}
	navTabs.get = function(){
		var ret ;
		sel.selectAll(".nav-item").filter(function(d){
			return d3.select(this).select(".nav-link").classed("active")
		}).each(function(d){ ret =d; })
		
		if(ret != undefined) return navTabs.text()(ret)
		else return undefined;
	}
	navTabs.update = function(_){
	  data = _;
		
	  sel.selectAll(".viz-navTabs").remove()
	  
	  sel.append("ul")
		.attr("class","nav nav-tabs viz-navTabs")
		.attr("role","tablist")
		.selectAll(".nav-item")
		.data(navTabs.data())
		.enter().append("li")
		.attr("class","nav-item")
		.append("a")
		.attr("class","nav-link")
		.attr("data-toggle","tab")
		.attr("href",navTabs.id())
		.attr("role","tab")
		.text(navTabs.text())
			
		return navTabs;
	}
    return navTabs;
  }
  viz.LL={
	tiles:{
		OpenStreetMap:{
			 WorldStreet:'http://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/{z}/{y}/{x}'
			,satellite:'http://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}'
			,attribution:'&copy; <a href="http://osm.org/copyright">OpenStreetMap</a> contributors'
		}
		,OpenTopoMap:{
			url:'http://{s}.tile.opentopomap.org/{z}/{x}/{y}.png'
			,attribution: 'Map data: &copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>, <a href="http://viewfinderpanoramas.org">SRTM</a> | Map style: &copy; <a href="https://opentopomap.org">OpenTopoMap</a> (<a href="https://creativecommons.org/licenses/by-sa/3.0/">CC-BY-SA</a>)'
		}
		,Thunderforest:{
			Cycle:'http://{s}.tile.thunderforest.com/cycle/{z}/{x}/{y}.png'
			,Transport:'http://{s}.tile.thunderforest.com/transport/{z}/{x}/{y}.png'
			,TransportDark:'http://{s}.tile.thunderforest.com/transport-dark/{z}/{x}/{y}.png'
			,TransportLandscape:'http://{s}.tile.thunderforest.com/landscape/{z}/{x}/{y}.png'
			,TransportOutdoors:'http://{s}.tile.thunderforest.com/outdoors/{z}/{x}/{y}.png'
			,TransportPioneer:'http://{s}.tile.thunderforest.com/pioneer/{z}/{x}/{y}.png'
			,attribution: '&copy; <a href="http://www.thunderforest.com/">Thunderforest</a>, &copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
		}
		,OpenMapSurfer:{
			Roads:'http://korona.geog.uni-heidelberg.de/tiles/roads/x={x}&y={y}&z={z}'
			,Grayscale:'http://korona.geog.uni-heidelberg.de/tiles/roadsg/x={x}&y={y}&z={z}'
			,attribution: 'Imagery from <a href="http://giscience.uni-hd.de/">GIScience Research Group @ University of Heidelberg</a> &mdash; Map data &copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
		}
		,Hydda:{
			Full:'http://{s}.tile.openstreetmap.se/hydda/full/{z}/{x}/{y}.png'
			,Base:'http://{s}.tile.openstreetmap.se/hydda/base/{z}/{x}/{y}.png'
			,attribution: 'Tiles courtesy of <a href="http://openstreetmap.se/" target="_blank">OpenStreetMap Sweden</a> &mdash; Map data &copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
		}
		,Stamen:{
			Toner:'http://stamen-tiles-{s}.a.ssl.fastly.net/toner/{z}/{x}/{y}.{ext}'
			,TonerBackground:'http://stamen-tiles-{s}.a.ssl.fastly.net/toner-background/{z}/{x}/{y}.{ext}'
			,TonerLite:'http://stamen-tiles-{s}.a.ssl.fastly.net/toner-lite/{z}/{x}/{y}.{ext}'
			,Watercolor:'http://stamen-tiles-{s}.a.ssl.fastly.net/watercolor/{z}/{x}/{y}.{ext}'
			,Terrain:'http://stamen-tiles-{s}.a.ssl.fastly.net/terrain/{z}/{x}/{y}.{ext}'
			,TerrainBackground:'http://stamen-tiles-{s}.a.ssl.fastly.net/terrain-background/{z}/{x}/{y}.{ext}'
			,attribution:  'Map tiles by <a href="http://stamen.com">Stamen Design</a>, <a href="http://creativecommons.org/licenses/by/3.0">CC BY 3.0</a> &mdash; Map data &copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
		}
		,Esri:{
			WorldStreetMap:'http://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/{z}/{y}/{x}'
			,WorldImagery:'http://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}'
			,WorldTerrain:'http://server.arcgisonline.com/ArcGIS/rest/services/World_Terrain_Base/MapServer/tile/{z}/{y}/{x}'
			,WorldShadedRelief:'http://server.arcgisonline.com/ArcGIS/rest/services/World_Shaded_Relief/MapServer/tile/{z}/{y}/{x}'
			,WorldPhysical:'http://server.arcgisonline.com/ArcGIS/rest/services/World_Physical_Map/MapServer/tile/{z}/{y}/{x}'
			,OceanBasemap:'http://server.arcgisonline.com/ArcGIS/rest/services/Ocean_Basemap/MapServer/tile/{z}/{y}/{x}'
			,NatGeoWorldMap:'http://server.arcgisonline.com/ArcGIS/rest/services/NatGeo_World_Map/MapServer/tile/{z}/{y}/{x}'
			,WorldGrayCanvas:'http://server.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Light_Gray_Base/MapServer/tile/{z}/{y}/{x}'
			,attribution: 'Tiles &copy; Esri &mdash; Source: Esri, National Geographic, DeLorme, NAVTEQ, USGS, Intermap, iPC, NRCAN, Esri Japan, METI, Esri China (Hong Kong), Esri (Thailand), TomTom, 2012'
		}
		,Here:{
			normalDay:'http://{s}.{base}.maps.cit.api.here.com/maptile/2.1/{type}/{mapID}/normal.day/{z}/{x}/{y}/{size}/{format}?app_id={app_id}&app_code={app_code}&lg={language}'
			,hybridDay:'http://{s}.{base}.maps.cit.api.here.com/maptile/2.1/{type}/{mapID}/hybrid.day/{z}/{x}/{y}/{size}/{format}?app_id={app_id}&app_code={app_code}&lg={language}'
			,attribution: 'Map &copy; 1987-2014 <a href="http://developer.here.com">HERE</a>'
		}
		,CartoDB:{
			Positron:'http://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png'
			,PositronNoLabels:'http://{s}.basemaps.cartocdn.com/light_nolabels/{z}/{x}/{y}.png'
			,PositronOnlyLabels:'http://{s}.basemaps.cartocdn.com/light_only_labels/{z}/{x}/{y}.png'
			,DarkMatter:'http://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png'
			,DarkMatterNoLabels:'http://{s}.basemaps.cartocdn.com/dark_nolabels/{z}/{x}/{y}.png'
			,DarkMatterOnlyLabels:'http://{s}.basemaps.cartocdn.com/dark_only_labels/{z}/{x}/{y}.png'
			,attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a> &copy; <a href="http://cartodb.com/attributions">CartoDB</a>'
		}
	}
  };
  
  function boxWhisker(a){
	var sa = a.filter(function(d){ return !isNaN(d);}).sort(d3.ascending), l = sa.length;
 
	if(l==0) return [];
	else if(l==1) return [sa[0],sa[0],sa[0],sa[0],sa[0]];
	else if(l==2) return [sa[0],sa[0],(sa[1]+sa[0])/2,sa[1],sa[1]];
	
	var q2=(l%2 ? sa[(l-1)/2] : (sa[l/2]+sa[l/2-1])/2);
	
	var q1i = Math.floor((l+1)/4-1);
	var q1=(l%4==3 ? sa[q1i] : (sa[q1i]+sa[q1i+1])/2);
	
	var q3i = Math.floor(3*(l+1)/4-1);
	var q3=(l%4==3 ? sa[q3i] : (sa[q3i]+sa[q3i+1])/2);
	var iqr = q3-q1;
	
	return [Math.max(sa[0],q1-1.5*iqr),q1,q2,q3,Math.min(sa[l-1],q3+1.5*iqr)];
  }
  function viz_compute_uscounties(){
    if(viz.maps.uscounties.stateByAbb !== undefined) return;
	
    viz.maps.uscounties.stateByFIPS ={};	
    viz.maps.uscounties.stateByAbb ={};
	  
    viz.maps.uscounties.states.forEach(function(d){
		viz.maps.uscounties.stateByFIPS[d.FP]=d;
		viz.maps.uscounties.stateByAbb[d.ABB]=d;
	});
  }
  function viz_d(d){ return d}
  function viz_d0(d){ return d[0]}
  function viz_d1(d){ return d[1]}
  function viz_x(d){ return d.x}
  function viz_y(d){ return d.y}
  function viz_height(d){ return d.height}
  function viz_width(d){ return d.width}
  function viz_height2(d){ return d.height/2}
  function viz_width2(d){ return d.width/2}
  function viz_key(d){ return d.key}
  function viz_value(d){ return d.value}
  
  function viz_assign(o){
	var x;
    return function(_){
      if(!arguments.length) return x;
      x = _;
      return o;
    }
  }
  function viz_assign_default(o,d){
	var x;
    return function(_){
	  if(!arguments.length) return typeof x !== "undefined" ? x : d ;
	  x = _;
	  return o;		
    }
  }

  function viz_reduceAngle(a){
    while(a>tau) a-=tau;
    while(a<0) a+=tau;
    return a;
  }
  function viz_polar(r, a){  return {x:r*Math.cos(a), y:r*Math.sin(a)};  }
  function viz_getratio(a0, p, m, h, tt, f){
	if(tt <= 0 || h <= 0) return 0;
    var a=a0.concat().sort(d3.ascending);
    var h0=h-a.length*p+(f? p: 0);
    var ret =[], r=0, t=0;
	
    d3.range(a.length).forEach( function(d){ 
      t =  (h0-m*d)/(tt-=(a[d-1]||0));
  	  r+=a[d]*t <= m ? 1 :0;
      ret.push(t);
    });
    return ret[r];
  }
  function viz_getbars(a, p, m, h0, h1){
    var x=h0, total = d3.sum(a);
    var r=viz_getratio(a, p, m, h1 - h0, total, false);
    var s = a.map(function(d){ 
      var v = r*d;
      var w =(v < m ? m :v)/2;
      x+=2*w+p;	
  	return {c:x-w, v:v, w:w, value:d, percent:d/(total||1)}
    });
    return s;
  }
  function viz_arc(x){
    function polar(r,t){ return [r*Math.cos(t), r*Math.sin(t)]; }
    var ss=polar(x[0],x[2]), se=polar(x[0],x[3]), es=polar(x[1],x[2]), ee=polar(x[1],x[3]);
    return ["M",ss,"A",x[0],x[0],"0",(x[3]-x[2] > pi?1:0),"1",se, "L",ee,
          "A",x[1],x[1],"0",(x[3]-x[2] > pi?1:0),"0",es, "z"].join(" ");
  }
  function viz_chord(rs, ss, se, re, es, ee) {
	  var pss=p(rs,ss), pse=p(rs,se), pes=p(re,es), pee=p(re,ee);
	
      return "M" + pss + arc(rs, pse, se - ss) 
	        + ((ss==es && se==ee) ? curve(pss,ss,se,re) : curve(pes,se,es,re) + arc(re, pee, ee - es) + curve(pss,ss,ee,rs)) + "Z";
	
      function arc(r, p, a) {  return "A" + r + "," + r + " 0 " + +(a > pi) + ",1 " + p;   }
	  function p(r,a){ return [r*Math.cos(a), r*Math.sin(a)]; }
      function eq(a, b){ return a.a0 == b.a0 && a.a1 == b.a1; }
      function curve(p1,a0,a1, r) {
        a1=a1+ (a1<a0?tau:0);
	    var a=a1-a0;
	    var t=1-(a > pi ? tau-a : a)/pi;
	    t=Math.pow(t,5);
	    var a2 = (a1+a0)/2 -(a1-a0>pi ? pi : 0);
        return "Q"+ (t*r* Math.cos(a2))+","+(t*r*Math.sin(a2))+" " + p1;
      }
    }
  function viz_schemeCategory10(){ return d3.scaleOrdinal().range(d3.schemeCategory10) }
  function viz_shiftarray(n,i){
    ret =[];
    for(var s=i; s>i-n; s--){
      ret.push(s<0? s+n : s);
    }
    return ret;
  }
  this.viz=viz;
}();

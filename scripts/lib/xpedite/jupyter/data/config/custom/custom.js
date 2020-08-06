/*******************************************************************
 * script for loading js files on jupyter startup
 *******************************************************************/

requirejs.config({
    appDir: ".",
    baseUrl: "js",
    paths: { 
      'jquery': ['/static/jquery-3.2.1.min'],
      'bootstrap': ['/static/bootstrap.min'],
      'd3': ['/static/d3.min'],
      'd3v4': ['/static/d3.v4.min'],
      'd3flot': ['/static/d3flot'],
      'flot': ['/static/jquery.flot.min'],
      'tablesorter': ['/static/jquery.tablesorter.min'],
      'tipsy': ['/static/jquery.tipsy'],
      'xpedite': ['/static/xpedite'],
      'sunburst': ['/static/sunburst'],
    },
    shim: {
      'bootstrap' : ['jquery'],
    }
});

require(['jquery', 'bootstrap', 'flot', 'tablesorter', 'tipsy', 'xpedite'], function($) {
  console.log("In custom js - require init block");
  return {};
});

function onSelectBenchmark(selectBox) {
  for(i=0; i<selectBox.length; i++)
  {
    var tpsTableCl = ".timePointStats-" + i;
    $(tpsTableCl).each(function(i, obj) {
      obj.style.display = "none";
    })
  }
  var tpsTableCl = ".timePointStats-" + selectBox.selectedIndex;
  $(tpsTableCl).each(function(i, obj) {
    obj.style.display = "table";
  })
}

function plotAccordingToChoices(datasets, placeholderId, choiceContainerId) {
  var data = [];
  var choiceContainer = $(choiceContainerId); 
  choiceContainer.find("input:checked").each(function () {
    var key = $(this).attr("name");
    if (key && datasets[key]) {
      data.push(datasets[key]);
    }
  });

  if (data.length > 0) {
    $.plot($(placeholderId),data, {});
  }
}

function onSelectSeries(e) {
  var constituentSelector = $(e.data.constituentSelectorId); 
  plotAccordingToChoices(e.data.seriesCollection[constituentSelector[0].selectedIndex], e.data.placeholderId, e.data.choiceContainerId);
}


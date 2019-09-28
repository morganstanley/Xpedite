function openPage(pageName) {
    var j, tablink;
    tablink = document.getElementsByClassName("tablink");
    for (j = 0; j < tablink.length; j++) {
        tablink[j].className = tablink[j].className.replace(" active","");
    }
    event.currentTarget.className += " active";
    var i, tabcontent;
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }
        document.getElementById(pageName).style.display = "block";
}

document.getElementById("live-tab-btn").click();

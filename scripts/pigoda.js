
var refresh = false;
var developer_mode = false;

function dprint(msg) {
	if (developer_mode) {
		console.log(msg);
	}
	return (msg);
}

function restore_from_cookie(cks) {
    //var cks = Cookies.getJSON('graphs_sorted');
    dprint(cks[0]);
    var objs = {};
    $(".panel").each(function() {
	var nbuf=$(this).find(".panel-heading").html();
	var pname=nbuf.split(/[ ]/)[0]
        objs[pname] = $(this).detach();
        dprint("Detached "+pname);
     });
    for (var n=0; n< cks.length ;n++) {
       dprint("restoring "+cks[n]);
       objs[cks[n]].appendTo("#graph_list");
       //$("#"+cks[n]+"_graph_panel").show();
    }
}

function refresh_imgs() {
       
       $(".panel-body").each(function() {
    
         var cur_jq_obj = $(this).find("img");
         var curimg = cur_jq_obj.attr("src");

         dprint("refreshing \""+curimg+"\"");
         cur_jq_obj.attr("src", curimg.split("?date=")[0]+"?date="+(new Date()).getTime());
       });
     	window.setTimeout(refresh_imgs, 60000);
}

$(document).ready(function() {
   var umode = Cookies.getJSON('user_modes');
   var graph_list_cookie = Cookies.getJSON('graphs_sorted');
   if (graph_list_cookie) {
	restore_from_cookie(graph_list_cookie);
   }
   if (umode && umode.developer) {
    $("#graph_buttons").show();
    developer_mode = true;
   } else {
     refresh_imgs();
   }

   $(".panel").dblclick(function() {
    });
   $("#refresh_btn").click(function () {
     refresh_imgs();
   });
   $("#save_btn").click(function () {
    var objs = [];
    refresh = false;
    $(".panel").each(function() {
	var v=$(this).find(".panel-heading").html();
	var sname=v.split(/[ ]/)[0]
	objs.push(sname);
    
    });
     Cookies.set('graphs_sorted', objs);
   }); /* save btn */
    $(function() {
      $("#graph_list").sortable({
        cursor: "move",
        opacity: 0.5
      });
    });
   var n = 0;
});

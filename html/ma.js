/* This JS file contains functions common to all TPS web-interface forms
 * 
 * (c) 2016 Tibbo Technology, Inc.
 */

var errmsgs = {
	0: "No error",
	1: "Field cannot be empty",
	2: "Not a valid IP address",
	3: "Not a valid host name",
	4: "Not a valid port number",
	5: "Password too long",
	6: "Passwords do not match",
   10: "PIN code must be 4 digits or empty",
   11: "Value too long",
   12: "Invalid characters",
}; 


var errcss = {
  fontSize: ".7rem",
  minWidth: ".7rem",
  padding: ".2rem .5rem",
  border: "1px solid rgba(212, 212, 212, .4)",
  borderRadius: "3px",
  boxShadow: "2px 2px 4px #555",
//  color: "#eee",
//  backgroundColor: "#111",
	color: "#ffffff",
	backgroundColor: "#ff6600",
  opacity: "0.85",
  zIndex: "32767",
  textAlign: "left"
};


function postData(src)
{
    $.ajax("save.html", args, function(reply) {
        //
    });
}


function collectFormArgs(btn)
{
    var res = {};
    var n, t;
    
    /* class 'cff' is for collectable form fields
     */
    //$(".cff").each(function(i, e){
	btn.closest("form").find(".cff").each(function(i, e) {
        n = $(this).attr("name");
		console.log(n);
        if(n === undefined || n.length == 0) {
            n = $(this).attr("id");
            if(n === undefined || n.length == 0)
                return;
        }
        t = $(this).attr("type");
        if("checkbox" == t) {
            res[n] = $(this).is(":checked"); //("true" == $(this).is(":checked"))? 1: 0;
        } else if("radio" == t) {
            res[n] = $("input[name=\""+n+"\"]:checked").val();
        } else if($(this).prop("tagName") == "SELECT") {
			if($(this).hasClass("sendall")) {
				var ol = [];
				$(this).find("option").each(function() {
					ol.push($(this).val());
				});
				console.log(ol);
				res[n] = ol.join(",");
			} else {
				res[n] = $(this).find("option:selected").val();
			}
		} else {
            res[n] = $(this).val();
        }
    });

	console.log(res);
    return res;
}


function showm(msg, cls)
{
    var m = msg || "";
    var d = $("#msg");
    
    d.removeClass().addClass(cls);
    if(m.length > 0)
       d.text(msg);
    d.fadeIn();
    
    setTimeout(function(){ d.fadeOut(); }, 10000);
}


function showMsg(msg)
{
    showm(msg, "msg");
}


function showError(msg)
{
    showm(msg, "err");
}


function parseResponse(args)
{
	var msg;

	$(".errors").hideBalloon().removeClass("errors");
    if("ok" == args["status"])
        showMsg();
    else if("error" == args["status"])
        showError(args["errmsg"]);
	else if("failed" == args["status"]) {
		console.log(args);
		var errs = args["errors"];
		var en, n;
		for(var k in errs) {
			if(errs.hasOwnProperty(k)) {
				en = parseInt(errs[k]);
				n = ".cff[name=" + k + "]";
				msg = errmsgs[en] || ("Error message #" + en);
				$(n).addClass("errors");
				$(n).showBalloon({contents:msg, position:'right', css:errcss});
			}
		}
	}
}


$(document).ready(function() { 
    $("input[type=submit]").click(function(ev){
		var me = $(this);

		if(window.location.protocol == "file:") {
			collectFormArgs($(this));
			ev.preventDefault();
			return false;
		}

        me.attr("disabled", "disabled");
		me.closest("form").find("#l").show();
        $.post("save.html", collectFormArgs($(this)), function(reply) {
            var res = {};
            try {
                res = $.parseJSON(reply);
            } catch(e) {
				showError("Malformed data received");
			}
            if(Object.keys(res).length > 0)
                parseResponse(res);
        })
		.always(function() {
			me.closest("form").find("#l").hide();
            me.removeAttr("disabled");
        })
        .fail(function() {
            showError("Communication error");
        })
		;		
    });
    
    $("#close").click(function(){
		$("#msg").fadeOut();
	});
});


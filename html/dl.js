var devId = null;
var bli = -1;
var ajax_url = "ajax_dl.html";
var is_local = (window.location.protocol == "file:");
var serial_number = "";


function blink(n)
{
	var i, c;

	for(i=1; i<5; ++i) {
		if(i<n)
			c = "numeric";
		else if(i>=n)
			c = "numeric-red";
		if(i==n)
			c += " blinker";
		$("#fl"+i).removeClass().addClass(c);
	}
}


function updatePage(data) 
{
	var hwaddr = data["hwaddr"];
	var st = data["st"] || 0;
	var hex = data["hex"] || "";
	var add;

	serial_number = hwaddr;
	if(data["status"] == "error") {
		$("#df").hide();
		$("#di").hide();
		$("#de").text(data["errmsg"]).show();
		blink(2);
		devId = null;
	} else {
		if(hwaddr.length > 0) {
			if(devId != hwaddr) {
				$("#di").hide();
				$("#de").hide();
				if(hex != "") {
					add = ", Tibbo Hexagon: " + hex;
					$("#nn").show();
					$("#st").hide();
					$("#sb").hide();
				} else {
					add = "";
					$("#nn").hide();
					$("#st").show();
					$("#sb").show();
				}
				$("#df").text(data["bus"] + " address: " + data["hwaddr"] + add).show();
				$("#st").val(st);
				blink(3);
				devId = hwaddr;
				if($("#bi").val() == "rs485") {
					$("#csnd").show();
				}
			}
		} else { 
			if(devId != "") {
				$("#df").hide();
				$("#de").hide();
				$("#di").show();
				blink(2);
				devId = "";
				$("#csnd").hide();
				$("#st").val(0);
			}
		}
	}
}


function fillBusNumbers()
{
	var bn = $("#bn");
	var o = "<option>";
	
	bn.find("option").remove();
	bn.append($(o).text("0").val("0"));
	if($("#bi").val() != "rs485") {
		bn.append($(o).text("1").val("1"));
		bn.append($(o).text("2").val("2"));
	}
	bn.val("0");
}


function currentBus()
{
	var bi = $("#bi").val();
	switch(bi) {
		case "i2c":
			return 0;
		case "1w":
		case "1wire":
		case "1-wire":
			return 1;
		case "rs485":
			return 2;
	}
	return -1;
}


function busScan()
{
	var busId = currentBus(); //parseInt($("#bi").val());
	var busN = parseInt($("#bn").val());

	if(is_local)
		return;
	
	if(isNaN(busId))
		busId = 0;

	if(isNaN(busN))
		busN = 0;
	
	$("#l").show();

	args = { "a": "dl", "bi": busId, "bn": busN };
	
	$.get(ajax_url, args, function(data) {
		var d = $.parseJSON(data);
		updatePage(d);
	})
	.fail(function(){
		updatePage({"status": "error", "errmsg": "Communication error"});
	})
	.always(function(){
		$("#l").hide();
	});
}


function refreshNow()
{
	clearInterval(bli);
	busScan();
	bli = setInterval(busScan, 5000);
}


function busChanged()
{
	$("#di").show();
	$("#df").hide();
	$("#de").hide();
	$("#l").hide();
	$("#st").val(0);
	$("#nn").hide();
	$("#st").show();
	$("#sb").show();
	blink(2);
	devId = null;
	refreshNow();
}


function busIndexChanged(src)
{
	fillBusNumbers();
	busChanged();
}


function assocChanged()
{
	blink(4);
}


function saveAssoc()
{
	var args;
	var busId = currentBus(); //parseInt($("#bi").val());
	var busN = parseInt($("#bn").val());
	var st = parseInt($("#st").val());
	var di;

	if(isNaN(busId))
		busId = 0;

	if(isNaN(busN))
		busN = 0;

	if(isNaN(st))
		st = 0;

	di = (null === devId)? "": devId;

	clearInterval(bli);
	bli = -1;

	$("#save").attr("disabled", "disabled").text("Saving...");
	$("#l").hide();
	$("#s").show();
	
	args = { "a": "s", "bi": busId, "bn": busN, "dev": di, "st": st };

	$.get(ajax_url, args, function(data) {
		var d = $.parseJSON(data);
		if(d["status"] == "ok") // ok
			showMsg(d["action"]);
	})
	.fail(function(){
		updatePage({"status":"error", "errmsg":"Communication error"});
	})
	.always(function(){
		$("#s").hide();
		$("#save").removeAttr("disabled").text("Save");
		if(bli != -1) {
			clearInterval(bli);
			bli = -1;
		}
		bli = setInterval(busScan, 5000);
	});
}


function clearAssocs()
{
	swal({
		title: "Confirm action",
		text: "Are you sure to remove all records from sensor association database?\n\nThis action cannot be undone!",
		showCancelButton: true,
	}).then(function(isConfirm) {
		if(isConfirm) {
			clearInterval(bli);
			bli = -1;
			args = { "a": "rst" };
			$("#s").show();
			$.get(ajax_url, args, function(data) {
				var d = $.parseJSON(data);
				if(d["status"] == "ok") // ok
					showMsg(d["action"]);
			})
			.fail(function(){
				updatePage({"status":"error", "errmsg":"Communication error"});
			})
			.always(function(){
				$("#s").hide();
				if(bli != -1) {
					clearInterval(bli);
					bli = -1;
				}
				bli = setInterval(busScan, 5000);
			});
		}
	});
}


function changeSN()
{
	var pat = /^([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})$/;
	var busN = parseInt($("#bn").val());

	if(isNaN(busN))
		busN = 0;
		
	swal({
		title: "Change Serial Number",
		text: "Enter new Serial Number for this sensor:",
		input: "text",
		inputValue: serial_number,
		showCancelButton: true,
		showLoaderOnConfirm: true,
		inputValidator: function(result) {
			return new Promise(function(resolve, reject) {
				swal.enableLoading();
				if(pat.test(result)) {
					resolve();
				} else {
					reject('Enter valid Serial Number');
				}
			});
		 }
	}).then(function(val) {
		if(val !== false) {
			var args = {a: "ssn", bi: currentBus(), bn: busN, osn: serial_number, nsn: val};
			console.log(args);
			$.get(ajax_url, args, function(data){
				var d = $.parseJSON(data);
				console.log(d);
				if(d["status"] == "ok") // ok
					showMsg(d["action"]);
			}).always(function(){
				swal.disableLoading();
			}).fail(function(){
				console.log("failed");
				swal("Error", "Failed to set sensor new serial number", "error");
			});
		}
	});
}


$(document).ready(function() {
	$("#nn").hide();
	$("#csnd").hide();
	$("#di").show();
	$("#df").hide();
	$("#de").hide();
	$("#l").hide();
	$("#s").hide();
	$("#ok").hide();
	//$("#st").attr("disabled", "disabled");
	//$("#save").attr("disabled", "disabled");

	fillBusNumbers();
	blink(2);
	
	bli = setInterval(busScan, 5000);

	$("#bi").change(busIndexChanged);
	$("#bn").change(busChanged);
	$("#st").change(assocChanged);
	$("#save").click(saveAssoc);
	$("#clr").click(clearAssocs);
	$("#csn").click(changeSN);
});
 
 
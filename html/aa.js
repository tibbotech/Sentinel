/**
 *
 */
var ajax_url = "/ajax_aa.html";


function pInt(s)
{
	var v = parseInt(s);
	return isNaN(v)? 0: v;
}


function pFloat(s)
{
	var v = parseFloat(s);
	return isNaN(v)? 0: v;
}


function collectControls()
{
	var f, r, res;
	var id = $("#sensor option:selected").val();

	res = { sid: id, info: $("#loc").val() };
	
	f = 0;
	if($("#rsms").is(":checked")) f |= 1;
	if($("#remail").is(":checked")) f |= 2;
	if($("#rsnmp").is(":checked")) f |= 4;
	if($("#rout0").is(":checked")) f |= 8;
	if($("#rout1").is(":checked")) f |= 16;
	if($("#ren").is(":checked")) f |= 32;

	res["rlo"] = pFloat($("#lr").val());
	res["rhi"] = pFloat($("#hr").val());

	res["rts"] = pInt($("#rsmt option:selected").val());
	res["rte"] = pInt($("#remt option:selected").val());
	res["rtt"] = pInt($("#rtrp option:selected").val());
	res["rf"] = f;

	f = 0;
	if($("#ysms").is(":checked")) f |= 1;
	if($("#yemail").is(":checked")) f |= 2;
	if($("#ysnmp").is(":checked")) f |= 4;
	if($("#yout0").is(":checked")) f |= 8;
	if($("#yout1").is(":checked")) f |= 16;
	if($("#yen").is(":checked")) f |= 32;

	res["ylo"] = pFloat($("#ly").val());
	res["yhi"] = pFloat($("#hy").val());

	res["yts"] = pInt($("#ysmt option:selected").val());
	res["yte"] = pInt($("#yemt option:selected").val());
	res["ytt"] = pInt($("#ytrp option:selected").val());
	res["yf"] = f;

	return res;
}


function save()
{
	var args = collectControls();
	
	args["a"] = "save";
	console.log(args);
	
	$.get(ajax_url, args, function(reply) {
		console.log(reply);
	});
}


function setupControls(data)
{
	var yz, rz, f;

	yz = data["y"];
	rz = data["r"];
	f = parseInt(yz["flags"]);
	
	$("#loc").val(data["info"]);

	$("#lr").val(parseFloat(rz["lo"]));
	$("#ly").val(parseFloat(yz["lo"]));
	$("#hy").val(parseFloat(yz["hi"]));
	$("#hr").val(parseFloat(rz["hi"]));

	$("#ysmt").val(parseInt(yz["t_sms"]));
	$("#rsmt").val(parseInt(rz["t_sms"]));
	$("#yemt").val(parseInt(yz["t_email"]));
	$("#remt").val(parseInt(rz["t_email"]));
	$("#ytrp").val(parseInt(yz["t_trap"]));
	$("#rtrp").val(parseInt(rz["t_trap"]));

	$("#ysms").prop("checked", 0 != (f & 1));
	$("#yemail").prop("checked", 0 != (f & 2));
	$("#ysnmp").prop("checked", 0 != (f & 4));
	$("#yout0").prop("checked", 0 != (f & 8));
	$("#yout1").prop("checked", 0 != (f & 16));
	$("#yen").prop("checked", 0 != (f & 32));
	
	f = parseInt(rz["flags"]);
	$("#rsms").prop("checked", 0 != (f & 1));
	$("#remail").prop("checked", 0 != (f & 2));
	$("#rsnmp").prop("checked", 0 != (f & 4));
	$("#rout0").prop("checked", 0 != (f & 8));
	$("#rout1").prop("checked", 0 != (f & 16));
	$("#ren").prop("checked", 0 != (f & 32));
}


function sensorChanged()
{
	var id = $("#sensor option:selected").val();
	var args = { "a": "get", "sid": id };

	$.get(ajax_url, args, function(reply) {
		try {
			data = $.parseJSON(reply);
		} catch(e) {
			console.log("Invalid reply:"+reply);
			return;
		}
		console.log(data);
		setupControls(data);
	});
}


$(document).ready(function() {
	$("#save").click(save);
	$("#sensor").change(sensorChanged);
	sensorChanged();
});


var VAL_TYPES = {
    0:"none",
    1:"humidity",
    2:"temperature",
    3:"temperature_and_humidity",
    4:"flood",
    5:"fire",
    6:"smoke",
    7:"movement",
    8:"acdc_current",
    9:"shock",
    10:"reed_switch",
    11:"switch",
    12:"pressure",
    13:"light",
    14:"acceleration_three_axis",
    15:"acceleration_six_axis",
    16:"bit",
    17:"byte",
    18:"word",
    19:"dword",
    20:"gas_presence_co",
    21:"gas_presence_co2",
    22:"gas_presence_methane",
    23:"alcohol"
};


var VAL_UNITS = {
    0:"none",
    1:"%RH",
    2:"&#8451;",
    3:"%1&nbsp;&#8451;,&nbsp;%2&nbsp;%RH",
    4:"",
    5:"",
    6:"",
    7:"",
    8:"",
    9:"%1, %2, %3 mG",
    10:"",
    11:"",
    12:"mmHg",
    13:"Lux",
    14:"m/s",
    15:"m/s",
    16:"",
    17:"",
    18:"",
    19:"",
    20:"",
    21:"",
    22:"",
    23:""
};


var BUSES = {
	0: "IO_NONE",
	1: "I<sup>2</sup>C",
	2: "SPI",
	3: "1-Wire",
	4: "Single&nbsp;Wire",
	5: "Dry&nbsp;Contact",
	6: "Wet&nbsp;Contact",
	7: "RS-232",
	8: "RS-422",
    9: "RS-485",
    10: "OUT"
};


function td(v, al)
{
	var as = al || "";
	if(as.length != 0)
		as = " align=\""+as+"\"";

	return "<td"+as+">" + v + "</td>\n";
}


function buildTable(d) 
{
	var tb = $("#st tbody");
	var i, j, s, t, z;
	var html, res;

	html = "";

	for(i=0; i<d.length; ++i) {
		s = d[i];
		z = parseInt(s["zone"]);

		html += "<tr";
		if(z == 2)
			html += " class=\"yellow\"";
		else if(z == 3)
			html += " class=\"red\"";
		html += ">"; 

		html += td (s["n"]);

		t = parseInt(s["bus"]);
		html += td(BUSES[t]);
		
		html += td(s["channel"]);

		html += td(s["addr"]);

		t = parseInt(s["param"]);
		html += td(VAL_TYPES[t]);

		if(s["value"] instanceof Array) {
			z = s["value"];
			res = VAL_UNITS[t];
			for(j=0; j<z.length; ++j)
				res = res.replace("%"+(j+1), z[j]);
		} else {
			res = s["value"] + " " + VAL_UNITS[t];
		}

		html += td(res, "right") ;

		html += td(s["info"]);
		
		html += td(s["zone"]);

		html += "</tr>\n";
	}

	tb.html(html);
}


function queryData() 
{
	var ajax_url = "monitor_update.html";
	args = {}
	$.get(ajax_url, args, function(data) {
		try {
			var d = $.parseJSON(data);
			buildTable(d);
		} catch(e) {
			console.log(e);
		}
	})
	.fail(function(){
		// Dummy
	})
	.always(function(){
		// Dummy
	});
}


$(document).ready(function(){
	queryData();
	setInterval(queryData, 5000);
});
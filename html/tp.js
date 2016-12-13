/* Functions for 'message_templates' web page
 */
var ajax_url = "ajax_tp.html";


function template_load(id)
{
	var args = {"a": "load", "id": id};

	$("#wheel").show();
	$.get(ajax_url, args, function(data) {
		var d = $.parseJSON(data);
		if(d["status"] == "ok") {
			console.log(d);
			$("#mt").val(d["title"]);
			$("#tt").text(d["body"]);
		} else {
		
		}
	})
	.fail(function() {
	})
	.always(function() {
		$("#wheel").hide();
	});
}


function template_new()
{
	swal({
		title: "New template",
		text: "Template name",
		input: "text",
		showCancelButton: true,
	}).then(function(text) {
		if(text === false)
			return;

		var args = {"a": "new", "dn": text};

		swal.disableInput();
		swal.enableLoading();

		$.get(ajax_url, args, function(data) {
			var d = $.parseJSON(data);
			$("#name").append(
				$("<option></option>").attr("value", d["id"]).attr("selected", "selected").text(d["dn"])
			);
			$("#mt").focus();
		})
		.fail(function() {
		})
		.always(function() {
		});
		
	});
}


function template_delete()
{
	var so = $("#name option:selected");
	swal({
		title: "Delete template",
		text: "Are you sure to delete template &laquo;<i>"+so.text()+"</i>&raquo;?",
		type: "question",
		showCancelButton: true,
		cancelButtonText: "No",
		confirmButtonText: "Yes, delete"
	}).then(function(ok){
		if(ok === true)
			so.remove();
	});
}


function template_save(ev)
{
	var id = parseInt($("#name option:selected").val());
	var fd = new FormData();
	
	ev.preventDefault();
	
	fd.append("f", "tpls");
	fd.append("a", "save");
	fd.append("id", id);
	fd.append("title", $("#mt").val());
	fd.append("body", $("#tt").val());
	
	$("#wheel2").show();

	$.ajax({
		url: ajax_url,
		type: "POST",
		data: fd,
		async: true,
		cache: false,
		contentType: false,
		processData: false,
	}).always(function(){
		$("#wheel2").hide();
	});
	
	return false;
}


function template_change()
{
	var key = $("#name option:selected").val();
	template_load(key);
}


$(document).ready(function(){
	template_change();
	$("#new").click(template_new);
	$("#del").click(template_delete);
	$("#save").click(template_save);
	$("#name").change(template_change);
});

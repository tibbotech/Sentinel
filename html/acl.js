function addIp()
{
    var t = $("#ip").val().trim();
    var ipre = /^([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})((\/[0-9]{1,2}))?$/i;
    
    if(!ipre.test(t)) {
        swal("Incorrect IP address", "You must specify either IP address or subnet in slash or colon notation", "error")
        return;
    }
    
    $("#list").append(
        $("<option>").text(t).val(t)
    );
    
	$("#ip").val("");
}


function delIp()
{
    $("#list option:selected").remove(); 
}


function ipChange()
{
    if(0 == $("#ip").val().length) {
        $("#add").attr("disabled", "disabled");
    } else {
        $("#add").removeAttr("disabled");
    }
}


$(document).ready(function() {
    $("#ip").on("keyup", ipChange).change(ipChange);
    $("#add").click(addIp);
    $("#del").click(delIp);
    ipChange();
});

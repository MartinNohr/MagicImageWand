<!DOCTYPE HTML>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title>MagicImageWand ImageConverter</title>
<style>
h1{
	text-align:center;
}

div.led-box label{
    text-align: center;
    padding: 10px;
    box-sizing: border-box;
    display: inline-block;
    border-radius: 4px;
    width: 300px;
    overflow: auto;
}

div.led-box{
    width: 300px;
    margin: 0 auto;
    text-align: center;
}

div.led-box select{
    border-radius: 4px;
}

div.dir-box label{
    text-align: center;
    padding: 10px;
    box-sizing: border-box;
    display: inline-block;
    border-radius: 4px;
    width: 300px;
    overflow: auto;
}

div.dir-box{
    width: 300px;
    margin: 0 auto;
    text-align: center;
    padding: 30px;
}

div.dir-box select{
    border-radius: 4px;
}

div.inv-box label{
    text-align: center;
    padding: 10px;
    box-sizing: border-box;
    display: inline-block;
    border-radius: 4px;
    width: 300px;
    overflow: auto;
}

div.inv-box{
    width: 300px;
    margin: 0 auto;
    text-align: center;
}

div.inv-box select{
    border-radius: 4px;
}

div.upload-box{
    width: 300px;
    margin: 0 auto;
    text-align: center;
}

div.upload-box label{
    background: #eaf9ff;
    text-align: center;
    padding: 30px;
    box-sizing: border-box;
    display: inline-block;
    border: 2px dashed #abe5ff;
    cursor: pointer;
    color: #3dc2fd;
    border-radius: 4px;
    width: 300px;
    overflow: auto;
}

div.upload-box input[type=file]{
    display: none;
}
.server-results img{
    width: 100%;
    height: auto;
}

</style>
</head>
<body>
<h1>MagicImageWand ImageConverter</h1>

<div class="led-box">
<label for="size">Number of LEDs</label>
<select name="size" id="size" form="size">
  <option value="144">144</option>
  <option value="288">288</option>
  <option value="72">72</option>
  <option value="100">100</option>
</select>  
</div>

<div class="dir-box">
<label for="dir">Direction</label>
<select name="dir" id="dir" form="dir">
  <!-- depending on the ImageMagick Version installed, it might be needed to switch cw and ccw -->
  <option value="270">counterclockwise</option>
  <option value="90">clockwise</option>
  <option value="0">no rotation</option>
</select>
</div>

<div class="inv-box">
<label for="inv">Invert Colors</label>
<select name="inv" id="inv" form="inv">
  <option value="no">no</option>
  <option value="yes">yes</option>
</select>
</div>

<div class="upload-box">
        <label for="image-file">Upload file</label>
	<input type="file" name="image-file" id="image-file" accept="image/*" />
	<div id="server-results"></div>
        <div id="loader-icon" style="display:none;"><img src="LoaderIcon.gif" /></div>
</div>

<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>
<script>
var ajax_url = "lighty.php";
var max_file_size = 22194304;	
var image_filters = /^(?:image\/bmp|image\/cis\-cod|image\/gif|image\/ief|image\/jpeg|image\/jpeg|image\/jpeg|image\/pipeg|image\/png|image\/tiff|image\/x\-cmu\-raster|image\/x\-cmx|image\/x\-icon|image\/x\-portable\-anymap|image\/x\-portable\-bitmap|image\/x\-portable\-graymap|image\/x\-portable\-pixmap|image\/x\-rgb|image\/x\-xbitmap|image\/x\-xpixmap|image\/x\-xwindowdump)$/i;

$("#image-file").change(function(){
                        $('#loader-icon').show();
	if(window.File && window.FileReader && window.FileList && window.Blob){
		if($(this).val()){
                        $('#loader-icon').show();
			var oFile = $(this)[0].files[0];
			var fsize = oFile.size; //get file size
			var ftype = oFile.type; // get file type
			var osize = document.getElementById("size").value;;	
			var odir = document.getElementById("dir").value;;
            var oinv = document.getElementById("inv").value;;	
			if(fsize > max_file_size){
				$('#loader-icon').hide();
				alert("File size can not be more than (" + max_file_size +") 22MB" );
				return false;
			}
			
			if(!image_filters.test(oFile.type)){
				$('#loader-icon').hide();
				alert("Unsupported file type!" );
				return false;
			}
			
			var mdata = new FormData();
                        $('#loader-icon').show();
			mdata.append('image_data', oFile);
			mdata.append('size', osize);
			mdata.append('dir', odir);
            mdata.append('inv', oinv);
				
			jQuery.ajax({
				type: "POST", // HTTP method POST
				processData: false,
				contentType: false,
				url: ajax_url, //Where to make Ajax call
				data: mdata, //Form variables
				dataType: 'json',
				success:function(response){
					if(response.type == 'success'){
					$('#loader-icon').hide();
						$("#server-results").html('<div class="success">' + response.msg + '</div>');
					}else{
					$('#loader-icon').hide();
						$("#server-results").html('<div class="error">' + response.msg + '</div>');
					}
				}
			});
		}
	}else{
		alert("Can't upload! Your browser does not support File API!</div>");
		return false;
	}
});	

</script>

</body>
</html>


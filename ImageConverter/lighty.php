<?php

array_map('unlink', glob("uploads/*.bmp")); 

$upload_dir = "/www/htdocs/w00e3eda/essl/static/lighty/uploads/"; //path to upload directory
$upload_dir_url = "https://www.essl.de/static/lighty/uploads/"; //url pointing to upload directory

//$image_height = 144; //max image height
//$image_width = 4000000; //max image width

//make sure uploaded file is not empty
if(!isset($_FILES['image_data']) || !is_uploaded_file($_FILES['image_data']['tmp_name'])){
	//generate an error json response
	$response = json_encode(
					array( 
						'type'=>'error', 
						'msg'=>'Image file is Missing!'
					)
				);
	print $response;
	exit;
}

//get image info from a valid image file
$image_info  = getimagesize($_FILES['image_data']['tmp_name']); 

//get mime type of the image
if($image_info){
	$image_type	= $image_info['mime']; //image mime
}else{
	print json_encode(array('type'=>'error', 'msg'=>'Invalid Image file !')); 
	exit;
}

//ImageMagick
$image = new Imagick($_FILES['image_data']['tmp_name']);

if ($_POST["inv"] == "yes"){
	$image->negateImage('true');
}

$image->rotateImage(new ImagickPixel(), $_POST["dir"]);
$image->thumbnailImage($_POST["size"], 0);
$image->setImageFormat('BMP3');

//save image file to destination directory
$results = file_put_contents ($upload_dir. $_FILES['image_data']['name'].'.bmp', $image);

//output success response
if($results){
print json_encode(array('type'=>'success', 'msg'=>'<a href="'.$upload_dir_url. $_FILES["image_data"]["name"].'.bmp'.'"> Download  <br /><img src="'.$upload_dir_url. $_FILES["image_data"]["name"].'.bmp'.'">'));
	exit;
}

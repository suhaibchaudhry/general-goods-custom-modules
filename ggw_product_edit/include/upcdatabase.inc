<?php
require_once 'XML/RPC.php';

function ggw_product_upc_fetch() {
	drupal_set_header("Content-Type: application/json");
	/*
	The following require is part of the PEAR package XML_RPC.
	There is another package called XML_RPC2, this is not it.
	This example takes a simple GET (page.php?upc=123456789012)
	and sends it to the site.  There should probably be some error checking.
	
	To install PEAR for your server you should do one of the following:
	a) follow any instructions for your distro
	b) check your distro's package manager
	c) see http://pear.php.net/manual/en/installation.getting.php
	
	Once you have PEAR, you can install this by either:
	a) using your distro's package manager
	b) issue 'pear install XML_RPC' from the command line as root/admin 
	*/

	$response = array();
	$rpc_key = 'dfb4a076233bd1b1728bfc27ac4a8ee70a91700f';	// Set your rpc_key here
	
	// Setup the URL of the XML-RPC service
	$client = new XML_RPC_Client('/xmlrpc', 'http://www.upcdatabase.com');
	
	// Construct the entire parameter list (an array) for the call.
	// The array contains a single XML_RPC_Value object, a struct.
	// The struct is constructed from a PHP associative array, and each
	// value needs to be an XML_RPC_Value object.
	$upcCode = $_POST['upcCode'];
	if(db_result(db_query("SELECT COUNT(*) FROM {uc_products} up WHERE up.model = '%s'", $upcCode)) > 0) {
		$nid = db_result(db_query("SELECT up.nid FROM {uc_products} up WHERE up.model = '%s'", $upcCode));
		//$response['error'] = true;
		$response['nid'] = $nid;
		$response['duplicate'] = true;
		//print json_encode($response);
		//exit;
	}
	
	$params = array( new XML_RPC_Value( array(
		'rpc_key' => new XML_RPC_Value($rpc_key, 'string'),
		'upc' => new XML_RPC_Value($upcCode, 'string'),
		), 'struct'));

	// Construct the XML-RPC request.  Substitute your chosen method name
	$msg = new XML_RPC_Message('lookup', $params);
	
	//Set debug info to true.  Useful for testing, shows the response from the server
	// $client->setDebug(1);
	
	//More debug info, create the payload before sending.
	//Not necessary to function, but useful to test
	// $msg->createPayload();
	
	//TEST Print the response to the screen for testing
	// echo "<pre>" . print_r($msg->payload, true) . "</pre><hr />";
	
	//Actually have the client send the message to the server.  Save response.
	$resp = $client->send($msg);
	
	//If there was a problem sending the message, the resp will be false
	if (!$resp)
	{
		//print the error code from the client and exit
		$response['error'] = true;
		$response['error_text'] = 'Communication Error';
		//echo 'Communication error: ' . $client->errstr;
		
		print json_encode($response);
		exit;
	}
	
	//If the response doesn't have a fault code, show the response as the array it is
	if(!$resp->faultCode())
	{
		//Store the value of the response in a variable
		$val = $resp->value();
		//Decode the value, into an array.
		$data = XML_RPC_decode($val);
		//Optionally print the array to the screen to inspect the values
		//echo "<pre>" . print_r($data, true) . "</pre>";
		if($data['status'] == 'success') {
			$response['error'] = false;
			$response['status'] = $data['status'];
			$response['description'] = $data['description'];
			$response['size'] = $data['size'];
		} else {
			$response['error'] = true;
			$response['error_text'] = 'Barcode not found';
		}
	}else{
		//If something went wrong, show the error
		//echo 'Fault Code: ' . $resp->faultCode() . "\n";
		//echo 'Fault Reason: ' . $resp->faultString() . "\n";
		$response['error'] = true;
		$response['error_text'] = $resp->faultString();
	}
	
	print json_encode($response);
}
?>
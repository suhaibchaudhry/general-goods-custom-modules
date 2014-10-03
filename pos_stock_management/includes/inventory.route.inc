<?php
function pos_stock_management_inventory_list() {
	$failed_response = array(
		'status' => false,
		'error' => 'Could not load inventory list.'
	);

	if(array_key_exists('request', $_POST)) {
		$request = json_decode($_POST['request']);
		$uid = _pos_api_expose_uid_from_token($request->token);

		if($uid) {
			$sql = "SELECT pism.se_id, pism.product_nid, pism.stock_adjustment, pism.vendor_uid, pism.unit_cost, pism.stock_comment, pism.reset, pism.timestamp FROM {pos_in_stock_management} pism LEFT JOIN {users} u ON pism.vendor_uid = u.uid LEFT JOIN {node} n on u.uid = n.uid WHERE 1 ORDER BY pism.se_id DESC";
			$pos_in_stocks = db_query($sql);
			$inventory_list = array();
			while($pos_in_stock = db_fetch_object($pos_in_stocks)) {
				$inventory_list[] = $pos_in_stock;
			}

			pos_api_expose_respond(array('status' => true, 'inventory_list' => $inventory_list), true);
		}
	}

	pos_api_expose_respond($failed_response);		
}
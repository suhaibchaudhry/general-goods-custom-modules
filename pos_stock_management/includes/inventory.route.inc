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
			$sql = "SELECT pism.se_id, pism.product_nid, pism.stock_adjustment, pism.vendor_uid, pism.unit_cost, pism.stock_comment, pism.reset, pism.timestamp FROM {pos_in_stock_management} pism LEFT JOIN {users} u ON pism.vendor_uid = u.uid LEFT JOIN {node} n on u.uid = n.uid WHERE 1 GROUP BY pism.product_nid ORDER BY pism.timestamp DESC";
			$pos_in_stocks = db_query($sql);
			$inventory_list = array();
			while($pos_in_stock = db_fetch_object($pos_in_stocks)) {
				$inventory_list[$pos_in_stock->product_nid] = _pos_stock_management_item_history($pos_in_stock->product_nid);
			}

			pos_api_expose_respond(array('status' => true, 'inventory_list' => $inventory_list), true);
		}
	}

	pos_api_expose_respond($failed_response);		
}

function pos_stock_management_lookup() {
	$failed_response = array(
		'scan' => false,
		'error' => 'Could not find item.'
	);
	module_load_include('inc', 'pos_api_expose', 'includes/products.route');

	if(array_key_exists('request', $_POST)) {
		$request = json_decode($_POST['request']);
		$token = $request->token;
		$product_nid = trim($request->product_nid);

		$uid = _pos_api_expose_uid_from_token($token);
		if($uid) {
			$sql = 'SELECT n.nid, n.vid, n.title, p.model, p.list_price, p.sell_price, f.filepath, ctp_wrap.field_prod_packing_value
					FROM {uc_products} p
					INNER JOIN {node} n ON n.vid = p.vid

					LEFT JOIN {content_field_image_cache} cfic ON n.vid = cfic.vid
					LEFT JOIN {files} f ON f.fid = cfic.field_image_cache_fid
					LEFT JOIN {content_type_product} ctp_wrap ON n.vid = ctp_wrap.vid

					WHERE n.nid = \'%d\' AND n.status > 0
					LIMIT 0, 1';

			$product = db_fetch_object(db_query($sql, $product_nid));
			$datum = _pos_api_expose_map_product_datum($product);

			if($product) {
				$response = array(
					'scan' => true,
					'product' => $datum
				);
			} else {
				$response = array(
					'scan' => false,
					'error' => 'Could not find item.'
				);
			}

			pos_api_expose_respond($response, true);
		}
	}

	pos_api_expose_respond($failed_response);
}
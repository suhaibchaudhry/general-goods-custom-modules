<?php
function pos_stock_management_inventory_list() {
	$failed_response = array(
		'status' => false,
		'error' => 'Could not load inventory list.'
	);

	if(array_key_exists('request', $_POST)) {
		$per_page = 30;
		$request = json_decode($_POST['request']);
		$uid = _pos_api_expose_uid_from_token($request->token);

		if($uid) {
			if(isset($_GET['page'])) {
				$page = $_GET['page'];
			} else {
				$page = 1;
			}

			$start = ($page-1)*$per_page;

			$sql = "SELECT COUNT(*) FROM (SELECT pism.se_id FROM {pos_in_stock_management} pism LEFT JOIN {users} u ON pism.vendor_uid = u.uid LEFT JOIN {node} n on u.uid = n.uid GROUP BY pism.product_nid) tab";
			$total_items = db_result(db_query($sql));

			$sql = "SELECT pism.se_id, pism.product_nid, pism.stock_adjustment, pism.vendor_uid, pism.unit_cost, pism.stock_comment, pism.reset, MAX(pism.timestamp) as stock_timestamp FROM {pos_in_stock_management} pism LEFT JOIN {users} u ON pism.vendor_uid = u.uid LEFT JOIN {node} n on u.uid = n.uid GROUP BY pism.product_nid ORDER BY stock_timestamp DESC LIMIT %d, %d";

			$pos_in_stocks = db_query($sql, $start, $per_page);
			$inventory_list = array();
			while($pos_in_stock = db_fetch_object($pos_in_stocks)) {
				$inventory_list[] = array('product_nid' => $pos_in_stock->product_nid, 'event_logs' => _pos_stock_management_item_history($pos_in_stock->product_nid), 'product' => _pos_stock_management_inline_embed($pos_in_stock->product_nid));
			}

			pos_api_expose_respond(array('status' => true, 'inventory_list' => $inventory_list, 'total_pages' => ceil($total_items/$per_page)), true);
		}
	}

	pos_api_expose_respond($failed_response);		
}

function _pos_stock_management_inline_embed($product_nid) {
	module_load_include('inc', 'pos_api_expose', 'includes/products.route');

	$sql = 'SELECT n.nid, n.vid, n.title, p.model, p.list_price, p.sell_price, f.filepath, ctp_wrap.field_prod_packing_value
					FROM {uc_products} p
					INNER JOIN {node} n ON n.vid = p.vid

					LEFT JOIN {content_field_image_cache} cfic ON n.vid = cfic.vid
					LEFT JOIN {files} f ON f.fid = cfic.field_image_cache_fid
					LEFT JOIN {content_type_product} ctp_wrap ON n.vid = ctp_wrap.vid

					WHERE n.nid = \'%d\' AND n.status > 0
					LIMIT 0, 1';

	$product = db_fetch_object(db_query($sql, $product_nid));
	return _pos_api_expose_map_product_datum($product);
}

function pos_stock_management_inventory_add_stock() {
	$failed_response = array(
		'status' => false,
		'error' => 'Could not submit inventory form.'
	);

	if(array_key_exists('request', $_POST)) {
		$request = json_decode($_POST['request']);
		$token = $request->token;
		$product_nid = $request->product_nid;
		$quantity_purchased = $request->quantity_purchased;
		$vendor = $request->vendor;
		$unit_cost = $request->unit_cost;
		$stock_comment = $request->stock_comment;
		$reset = $request->reset;

		$uid = _pos_api_expose_uid_from_token($token);
		if($uid) {
			db_query("INSERT INTO {pos_in_stock_management} (product_nid, stock_adjustment, vendor_uid, unit_cost, stock_comment, reset, `timestamp`) VALUES (%d, %d, %d, '%s', '%s', %d, '%s')", $request->product_nid, $request->quantity_purchased, $request->vendor, $request->unit_cost, $request->stock_comment, $request->reset, time());

			pos_api_expose_respond(array(
					'status' => true,
					'message' => 'Stock has been updated successfully.'
				), true);
		}
	}

	pos_api_expose_respond($failed_response);
}

function pos_stock_management_get_form_plain() {
	$failed_response = array(
		'status' => false,
		'error' => 'Could not load inventory form.'
	);

	if(array_key_exists('request', $_POST)) {
		$request = json_decode($_POST['request']);
		$token = $request->token;
		$product_nid = trim($request->product_nid);

		$uid = _pos_api_expose_uid_from_token($token);
		if($uid) {
			if($product_nid) {
				$node = node_load($product_nid);
				$form_markup = drupal_get_form('pos_stock_management_edit_plain_form', $node);

				pos_api_expose_respond(array(
					'status' => true,
					'form_markup' => $form_markup
				), true);
			} else {
				pos_api_expose_respond($failed_response, true);
			}
		}
	}

	pos_api_expose_respond($failed_response);
}

function pos_stock_management_edit_plain_form($form_state, $node) {
  $form = array();
  
  $form['nid'] = array(
  	'#type' => 'value',
	'#value' => $node->nid
  );

  $form['new_stock'] = array(
	'#type' => 'fieldset',
	'#title' => t('New Stock Addition'), 
	'#collapsible' => FALSE
  );
  
  $form['new_stock']['vendor'] = array(
	'#type' => 'select',
	'#title' => t('Supplying Vendor'), 
	'#default_value' => 0,
	'#options' => pos_stock_management_vendors($node->nid)
  );
  
  $form['new_stock']['quantity_purchased'] = array(
  	'#type' => 'textfield',
	'#title' => t('Quantity Purchased'), 
	'#size' => 9,
	'#maxlength' => 9,
	'#required' => TRUE
  );

  $form['new_stock']['unit_cost'] = array(
  	'#type' => 'textfield', 
	'#title' => t('Unit Cost'), 
	'#size' => 20,
	'#maxlength' => 128,
	'#required' => TRUE
  );
  
  $form['new_stock']['stock_comment'] = array(
  	'#type' => 'textarea',
	'#title' => t('Stock Comment')
  );

  $form['new_stock']['reset'] = array(
  	'#type' => 'checkbox',
  	'#default_value' => 0,
  	'#title' => t('Reset Stock')
  );
  
  $form['new_stock']['add'] = array(
  	'#type' => 'submit',
	'#value' => t('Add To Stock')
  );
  
  $form['stock_history'] = array(
	'#type' => 'fieldset',
	'#title' => t('Stock Status and History'), 
	'#collapsible' => FALSE
  );
  
  $form['stock_history']['purchase_table'] = array(
  	'#type' => 'markup',
	'#value' => pos_stock_management_history($node->nid)
  );

  return $form;
}
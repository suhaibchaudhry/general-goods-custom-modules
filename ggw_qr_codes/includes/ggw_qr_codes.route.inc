<?php
function ggw_qr_codes_generate_store() {
	drupal_add_css(drupal_get_path('module', 'ggw_qr_codes').'/css/ggw_qr_codes.css');
	$content .= '<div class="qr-queue">';
	$content .= '<h2>Print Queued Labels:</h2>';
	$content .= ggw_qr_codes_generate_label_queue();
	$content .= '</div>';

	$tree = taxonomy_get_tree(7, 0, -1, 1);
	$content .= '<div class="qr-by-dept">';
	$content .= '<h2>Print by Departments:</h2>';

	$content .= ggw_qr_codes_theme_taxonomy_nested_tree($tree);
	$content .= '</div>';
	
	return $content;
}

function ggw_qr_codes_generate_label_queue() {
	$header = array('Queue #', 'Product Name');
	$rows = ggw_qr_code_queue_list();	

	$content = theme('table', $header, $rows);
	$content .= drupal_get_form('ggw_qr_code_label_form');
	return $content;
}

function ggw_qr_code_label_form() {
	$form = array();

	$form['flush'] = array(
		'#type' => 'checkbox',
		'#title' => 'Clear Queue'
	);


	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => 'Generate Labels'
	);

	return $form;
}

function ggw_qr_code_label_form_submit($form, $form_state) {
	if($form_state['values']['flush']) {
		db_query("DELETE FROM ggw_product_label_queue WHERE 1");
	}

	ggw_qr_codes_print_large_labels();
}

function ggw_qr_code_queue_list() {
	$items = _ggw_qr_code_queue_list();
	$rows = array();
	while($item = db_fetch_object($items)) {
		$rows[] = array(
			$item->qid,
			l($item->title, 'node/'.$item->nid)
		);
	}

	return $rows;
}

function _ggw_qr_code_queue_list() {
	$sql = "SELECT g.qid, n.nid, n.title, up.model FROM ggw_product_label_queue g INNER JOIN node n ON n.nid = g.product_nid INNER JOIN uc_products up ON n.vid = up.vid";
	$items = db_query($sql);

	return $items;
}

function ggw_qr_codes_print_large_labels() {
	$item_list = _ggw_qr_code_queue_list();
	$items = array();
	while($item = db_fetch_object($item_list)) {
		$items[] = array(
			'name' => $item->title,
			'barcode' => $item->model
		);
	}

	$header = 'Shelf Label Queue';
	ggw_qr_codes_generate($items, $header, "No products were present in the shelf label queue.");
}

function ggw_qr_codes_generate_store_shelf($tid) {
	$header = '';
	if(is_numeric($tid)) {
		$items = ggw_qr_codes_get_store_prods($tid);
		$term = taxonomy_get_term($tid);
		$header = 'Product Department: '.$term->name;
	} else {
		$items = array();
	}

	ggw_qr_codes_generate($items, $header, "No products were found in database.");
}

function ggw_qr_codes_generate_user($uid) {
	if(is_numeric($uid)) {
		$items = ggw_qr_codes_get_cust_prods($uid);	
	} else {
		$items = array();
	}
	
	ggw_qr_codes_generate($items, '', "No products were found in customer's purchase history.");
}

function ggw_qr_codes_scan() {
	drupal_add_css(drupal_get_path('module', 'ggw_qr_codes').'/css/ggw_qr_jgrowl.css');
	drupal_add_js(drupal_get_path('module', 'ggw_qr_codes').'/js/ggw_qr_jgrowl.js');
	drupal_add_js(drupal_get_path('module', 'ggw_qr_codes').'/js/ggw_qr_codes.js');
	drupal_add_css(drupal_get_path('module', 'ggw_qr_codes').'/css/ggw_qr_scanner.css');

	$content = '<div class="item-scans">';
	$content .= '<div class="scan-input"><input type="text" maxlength="100" name="barcode"  size="20" value="" autocomplete="off" class="form-text barcode-input" /></div>';
	$content .= '<div class="item-list"></div>';
	$content .= drupal_get_form('ggw_qr_codes_gen_form');
	$content .= '</div>';
	
	return $content;
}

function ggw_qr_codes_item_details() {
	drupal_set_header('Content-Type: application/json');
	$barcode = $_POST['item_barcode'];
	if($barcode) {
		$item = db_fetch_object(db_query("SELECT up.nid, n.title, f.filepath, up.model, ctp.field_prod_unit_barcode_value FROM uc_products up LEFT JOIN content_field_image_cache cfic ON up.vid = cfic.vid LEFT JOIN files f ON cfic.field_image_cache_fid = f.fid LEFT JOIN node n ON up.vid = n.vid LEFT JOIN content_type_product ctp ON n.vid = ctp.vid WHERE up.model = '%s' OR ctp.field_prod_unit_barcode_value = '%s'", $barcode, $barcode));
		if($item) {
			print json_encode(array('status' => true, 'barcode' => (string)$item->model, 'ubarcode' => (string)$item->field_prod_unit_barcode_value, 'nid' => $item->nid, 'title' => $item->title, 'image' => imagecache_create_url('product_list', $item->filepath)));
		} else {
			$item = db_fetch_object(db_query("SELECT cfad.nid, n.title, f.filepath, ctp.field_prod_unit_barcode_value, up.model FROM content_field_additional_barcodes cfad LEFT JOIN content_field_image_cache cfic ON cfad.vid = cfic.vid LEFT JOIN files f ON cfic.field_image_cache_fid = f.fid LEFT JOIN node n ON cfad.vid = n.vid LEFT JOIN uc_products up ON n.vid = up.vid LEFT JOIN content_type_product ctp ON n.vid = ctp.vid WHERE cfad.field_additional_barcodes_value = '%s' ", $barcode));
			if($item) {
				print json_encode(array('status' => true, 'barcode' => (string)$item->model, 'ubarcode' => (string)$item->field_prod_unit_barcode_value, 'nid' => $item->nid, 'title' => $item->title, 'image' => imagecache_create_url('product_list', $item->filepath), 'addl' => $barcode));
			} else {
				print json_encode(array('status' => false));
			}
		}
	} else {
		print json_encode(array('status' => false));	
	}
}

function ggw_qr_codes_update_unit_barcode() {
	drupal_set_header('Content-Type: application/json');
	$barcode = $_POST['barcode'];
	$nid = $_POST['nid'];
	if(!empty($barcode) && !empty($nid)) {
		$barcodes = db_result(db_query("SELECT COUNT(*) FROM uc_products WHERE model = '%s'", $barcode));
		$addls = db_result(db_query("SELECT COUNT(*) FROM content_field_additional_barcodes WHERE field_additional_barcodes_value = '%s'", $barcode));
		$unitbs = db_result(db_query("SELECT COUNT(*) FROM content_type_product WHERE field_prod_unit_barcode_value = '%s'", $barcode));

		if($barcodes == 0 && $addls == 0 && $unitbs == 0) {
			$node = node_load($nid);
			$node->field_prod_unit_barcode[0]['value'] = $barcode;
			node_save($node);
			print json_encode(array('status' => true, 'message' => 'Successly changed unit barcode for <strong>'.$node->title.'</strong> to <strong>'.$barcode.'</strong>.'));
		} else {
			print json_encode(array('status' => true, 'message' => 'New barcode provided is already in use.'));
		}
	} else {
		print json_encode(array('status' => false, 'message' => 'Invalid RPC invocations.'));
	}
}
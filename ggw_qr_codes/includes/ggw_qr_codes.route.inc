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
	$sql = "SELECT g.qid, n.nid, n.title, up.model, ctp.field_prod_packing_value FROM ggw_product_label_queue g INNER JOIN node n ON n.nid = g.product_nid INNER JOIN uc_products up ON n.vid = up.vid INNER JOIN content_type_product ctp ON n.vid = ctp.vid";
	$items = db_query($sql);

	return $items;
}

function ggw_qr_codes_print_large_labels() {
	$item_list = _ggw_qr_code_queue_list();
	$items = array();
	while($item = db_fetch_object($item_list)) {
		$node = node_load($item->nid);
		$image_path = $node->field_image_cache[0]['filepath'];
		if(empty($image_path) || !file_exists($image_path)) {
			$image_path = 'sites/general-goods.com/files/imagefield_default_images/notfound_0.png';
		}

		$items[] = array(
			'name' => $item->title,
			'barcode' => $item->model,
			'packaging' => $item->field_prod_packing_value,
			'sell_price' => uc_currency_format($node->sell_price, false),
			'image_path' => $image_path
		);
	}

	$header = 'Shelf Label Queue';
	ggw_qr_codes_generate_large($items, $header, "No products were present in the shelf label queue.");
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

function ggw_qr_codes_gen_form() {
	$form = array('#attributes' => array('target' => '_blank'));
	
	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => 'Generate Labels'
	);
	
	return $form;
}

function ggw_qr_codes_gen_form_submit($form, $form_state) {
	$items = array();
	$items_posted = $form['#post']['item'];
	//Remove Duplicates
	//Add Pictures
	if(is_array($items_posted)) {
		$items_posted = array_unique($items_posted);
		$i = 0;
		foreach($items_posted as $item) {
			$product = db_fetch_object(db_query("SELECT n.title, up.model FROM node n INNER JOIN uc_products up ON n.vid = up.vid WHERE n.nid = '%d'", $item));
			$items[$i]['name'] = ggw_qr_codes_truncate($product->title, 35);
			$items[$i]['barcode'] = $product->model;
			$i++;
			
		}
	}

	ggw_qr_codes_generate($items, $header, "No products were scanned.");
}

function ggw_qr_codes_generate($items, $header = '', $no_found_message = '') {
	//require_once(libraries_get_path('tcpdf') .'/config/lang/eng.php');
	require_once(libraries_get_path('tcpdf') .'/tcpdf.php');
	//require_once(libraries_get_path('fpdi') .'/fpdi.php');
	
	//module_load_include('inc', 'ggw_qr_codes', 'includes/pdf');
	
	$margin = array('left' => .27, 'top' => .5, 'right' => .31);
	$box = array('w' => 2.625, 'h' => 1, 'margin' => 0);
	
	$style = array(
    	'border' => 0,
   		'vpadding' => 0.19685,
    	'hpadding' => 0.19685,
    	'fgcolor' => array(0,0,0),
    	'bgcolor' => false, //array(255,255,255)
    	'module_width' => 3, // width of a single module in points
    	'module_height' => 3 // height of a single module in points
	);
	
	// initiate PDF
	$pdf = new TCPDF();
	//$pdf->SetMargins(PDF_MARGIN_LEFT, 40, PDF_MARGIN_RIGHT);
	//$pdf->SetAutoPageBreak(true, 40);
	$pdf->setFontSubsetting(false);
	
	$pdf->setTextColor(0, 0, 0);
	
	$pdf->setPageUnit('in');

	//$pdf->SetMargins(.27, .5, .31);
	// add a page
	$pdf->startPage('P', 'Letter');

	$pdf->setDrawColor(0, 0, 0);
	$pdf->SetFont("helvetica", "N", 9);
	
	if(count($items) > 0) {
		$i = 0;
		foreach($items as $item) {
			$row = floor($i/3);
			$col = $i%3;
			
			if($col == 0) {
				$pad = 0;
			} elseif($col == 1) {
				$pad = 0.11811;	
			} else {
				$pad = 0.11811*2;	
			}
			
			$x = $pad+$margin['left']+($col*($box['w']+$box['margin']));
			$y = 0.03+$margin['top']+($row*$box['h']);
			
			
			$pdf->setPageUnit('mm');
			$pdf->writeHTMLCell(ggw_qr_codes_InToMM($box['w'])-25, ggw_qr_codes_InToMM($box['h'])-3, ggw_qr_codes_InToMM($x)+25, ggw_qr_codes_InToMM($y)+3, '<strong>'.$item['name'].'</strong>'.'<br />'.'<strong>UPC: </strong>'.$item['barcode'], 0);
			$pdf->setPageUnit('in');
			
			//$pdf->Rect($x, $y, $box['w'], $box['h'], '', 'LTRB', array(0, 0, 0));
			
			$pdf->write2DBarcode($item['barcode'], 'QRCODE,L', $x, $y, 0, 0, $style, 'N');
			
			$i++;
			
			if($i == 30) {
				$i = 0;
				
				$pdf->setPageUnit('mm');
				$pdf->writeHTMLCell(ggw_qr_codes_InToMM(5)-25, ggw_qr_codes_InToMM(1)-3, 8, 4, '<strong>'.$header.'</strong>', 0);
				$pdf->setPageUnit('in');
				
				$pdf->AddPage();
			}
		}
	} else {
		$pdf->SetFont("helvetica", "N", 12);
		$pdf->setPageUnit('mm');
		$pdf->MultiCell(ggw_qr_codes_InToMM(4.8), 0, $no_found_message, 0, 'C', FALSE, 1, 0, 50);
		$pdf->setPageUnit('in');
	}

	$pdf->Output('customer_labels.pdf', 'D');	
}

function ggw_qr_codes_generate_large($items, $header = '', $no_found_message = '') {
	//require_once(libraries_get_path('tcpdf') .'/config/lang/eng.php');
	require_once(libraries_get_path('tcpdf') .'/tcpdf.php');
	//require_once(libraries_get_path('fpdi') .'/fpdi.php');
	
	//module_load_include('inc', 'ggw_qr_codes', 'includes/pdf');
	
	$margin = array('left' => .27, 'top' => .5, 'right' => .31);
	$box = array('w' => 4, 'h' => 2.5, 'margin' => 0);
	
	$style = array(
    	'border' => 0,
   		'vpadding' => 0.19685,
    	'hpadding' => 0.19685,
    	'fgcolor' => array(0,0,0),
    	'bgcolor' => false, //array(255,255,255)
    	'module_width' => 5, // width of a single module in points
    	'module_height' => 5 // height of a single module in points
	);
	
	// initiate PDF
	$pdf = new TCPDF();
	//$pdf->SetMargins(PDF_MARGIN_LEFT, 40, PDF_MARGIN_RIGHT);
	//$pdf->SetAutoPageBreak(true, 40);
	$pdf->setFontSubsetting(false);
	
	$pdf->setTextColor(0, 0, 0);
	
	$pdf->setPageUnit('in');

	//$pdf->SetMargins(.27, .5, .31);
	// add a page
	$pdf->startPage('P', 'Letter');

	$pdf->setDrawColor(0, 0, 0);
	$pdf->SetFont("helvetica", "N", 9);
	
	if(count($items) > 0) {
		$i = 0;
		foreach($items as $item) {
			$row = floor($i/2);
			$col = $i%2;
			
			if($col == 0) {
				$pad = 0;
			} elseif($col == 1) {
				$pad = 0.11811;	
			} else {
				$pad = 0.11811*2;	
			}
			
			$x = $pad+$margin['left']+($col*($box['w']+$box['margin']));
			$y = 0.03+$margin['top']+($row*$box['h']);
			
			
			$pdf->setPageUnit('mm');
			$pdf->writeHTMLCell(ggw_qr_codes_InToMM($box['w'])-40, ggw_qr_codes_InToMM($box['h'])-3, ggw_qr_codes_InToMM($x)+40, ggw_qr_codes_InToMM($y)+3, '<strong>'.$item['name'].'</strong>'.'<br /><br />'.'<strong>UPC: </strong>'.$item['barcode'].'<br /><strong>PCKG: </strong>'.$item['packaging'].'<br /><br /><span style="font-size: 15pt;"><strong>PRICE: </strong>'.$item['sell_price'].'</span><br /><br />'.'<img height="50" src="'.base_path().$item['image_path'].'" alt="" />', 0);
			$pdf->setPageUnit('in');
			
			//$pdf->Rect($x, $y, $box['w'], $box['h'], '', 'LTRB', array(0, 0, 0));
			
			$pdf->write2DBarcode($item['barcode'], 'QRCODE,L', $x, $y, 0, 0, $style, 'N');
			
			$i++;
			
			if($i == 8) {
				$i = 0;
				
				$pdf->setPageUnit('mm');
				$pdf->writeHTMLCell(ggw_qr_codes_InToMM(5)-25, ggw_qr_codes_InToMM(1)-3, 8, 4, '<strong>'.$header.'</strong>', 0);
				$pdf->setPageUnit('in');
				
				$pdf->AddPage();
			}
		}
	} else {
		$pdf->SetFont("helvetica", "N", 12);
		$pdf->setPageUnit('mm');
		$pdf->MultiCell(ggw_qr_codes_InToMM(4.8), 0, $no_found_message, 0, 'C', FALSE, 1, 0, 50);
		$pdf->setPageUnit('in');
	}

	$pdf->Output('shelf_labels.pdf', 'D');	
}
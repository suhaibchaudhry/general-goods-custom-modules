<?php
function _pos_search_features_product_search_data() {
	$char_limit = 1;
	$item_number = $_POST['item_number'];
	$item_description = $_POST['item_description'];
	$product_department = $_POST['product_department'];
	$sorts = array();

	if(_pos_search_features_input_size_validate($char_limit, $item_number, $item_description) && $product_department == '0') {
		print '<p>Please type in atleast '.$char_limit.' character(s) for atleast one search criteria to view results.</p>';
	} else {
		$wheres = array();
		
		$wheres[] = "n.type = 'product'";
		
		_pos_search_features_construct_like_wheres($wheres, array(
			'n.title' => $item_description,
			'p.model' => $item_number
		));
		
		$sorts[] = 'n.title ASC';
		
		if($product_department != '0') {
			$wheres[] = "t.tid = '".db_escape_string($product_department)."'";
			$sorts[] = 't.tid ASC';
		}

		if(!empty($wheres)) {
			$fieldMap = array(
				'{node} n' => array('n.title'),
				'{uc_products} p ON p.vid = n.vid' => array('p.model', 'p.sell_price'),
				'{content_type_product} ctp ON n.vid = ctp.vid' => array('ctp.field_prod_packing_value', 'ctp.field_prod_purchase_unit_value', 'ctp.field_prod_pcs_per_unit_value'),
				'{term_node} t ON n.vid = t.vid' => array('t.tid'),
				'{term_data} td ON t.tid = td.tid' => array('td.name AS tname')//,
				//'{cache_stock_count} csc ON n.nid = csc.product_nid' => array('csc.stock_value')
			);
			
			$query = _pos_search_features_construct_query($fieldMap, $wheres, $sorts);
		
			$data = array();
			$results = db_query($query);
			$header = array('UPC Barcode', 'Product Description', 'Department', 'Price', 'Stock Available', 'Packing', 'Purchase Unit', 'PCS. Per Unit');
			while($result = db_fetch_object($results)) {
				$data[] = array('<a href="#" class="pos-search-select" rel="'.$result->model.'">'.$result->model.'</a>', $result->title, $result->tname, uc_currency_format($result->sell_price), (int)$result->stock_value, $result->field_prod_packing_value, $result->field_prod_purchase_unit_value, $result->field_prod_pcs_per_unit_value);
			}

			print theme('table', $header, $data);
		}
	}
}
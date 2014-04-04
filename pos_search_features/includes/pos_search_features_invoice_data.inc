<?php
function _pos_search_features_invoice_search_data() {
	$char_limit = 2;
	$invoice_number = $_POST['invoice_number'];
	$invoice_date = $_POST['invoice_date'];
	$account_number = $_POST['account_number'];
	$company_name = $_POST['company_name'];
	
	if(_pos_search_features_input_size_validate($char_limit, $invoice_number, $invoice_date, $account_number, $company_name)) {
		print '<p>Please type in atleast '.$char_limit.' characters for atleast one search criteria to view results.</p>';
	} else {
		$wheres = array();
		
		_pos_search_features_construct_like_wheres($wheres, array(
			'o.order_id' => $invoice_number,
			'u.name' => $account_number,
			'n.title' => $company_name
		));
		
		if(!empty($invoice_date)) {
			$ts = strtotime($invoice_date);
			$wheres[] = 'o.created > '.($ts-86400).' AND o.created < '.($ts+86400); 
		}

		if(!empty($wheres)) {
			$fieldMap = array(
				'{uc_orders} o' => array('o.order_id', 'o.created'),
				'{users} u ON u.uid = o.uid' => array('u.name'),
				'{node} n ON u.uid = n.uid' => array('n.title'),
			);
			
			$query = _pos_search_features_construct_query($fieldMap, $wheres);
		
			$data = array();
			$results = db_query($query);
			$header = array('Invoice #', 'Invoice Date', 'Account ID', 'Company Name');
			while($result = db_fetch_object($results)) {
				if(empty($result->name)) {
					$result->name = 'Walk-in';
				}
				$data[] = array('<a href="#" class="pos-search-select" rel="'.$result->order_id.'OR'.'">'.$result->order_id.'</a>', date('m/d/y', $result->created), $result->name, $result->title);
			}

			print theme('table', $header, $data);
		}
	}
}
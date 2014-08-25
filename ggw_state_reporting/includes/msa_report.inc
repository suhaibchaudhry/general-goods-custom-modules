<?php
function ggw_state_reporting_download_msa($start_date, $end_date) {
	//header("Content-Type: text/plain");
	header('Content-disposition: attachment;filename=weekly_msa_report.txt');

	$company = 'GENERAL GOODS WHOLESALE';
	$address = '8000 HARWIN, SUITE 200 HOUSTON, TX 77036';

	$msa_number = '14047001';
	$tobacco_id = '93044639';
	$phone = '7137803636';
	$fax = '7137801718';

	$start_date_r = date('Ymd', $start_date);
	$end_date_r = date('Ymd', $end_date);
	$date_r = date('Ymd');

	$report = '';

	//MSA Report Header
	$report .= ggw_state_reporting_msa_cell('HID'.$msa_number.'TOB', 16);
	$report .= ggw_state_reporting_msa_cell('W'.$start_date_r.$company, 41);
	$report .= ggw_state_reporting_msa_cell($address, 90);
	$report .= ggw_state_reporting_msa_cell('TOBACO ID: '.$tobacco_id, 84);
	$report .= ggw_state_reporting_msa_cell($phone, 15);
	$report .= ggw_state_reporting_msa_cell($fax, 70);
	$report .= '000100000002'.$date_r.'1'."\n";

	$total_products = 0;
	$total_customers = 0;
	$total_purchases = 0;
	$total_amount = 0;
	$total_qty = 0;

	$report .= ggw_state_reporting_msa_get_inventory_report($total_products);
	$report .= ggw_msa_generate_customer_report($start_date, $end_date, $total_customers, $total_purchases, $total_amount, $total_qty);

	//Totals
	$report .= 'TOT14047001'.$end_date_r.ggw_state_reporting_msa_cell($total_products, 8, STR_PAD_LEFT, '0');
	$report .= '  ';
	$report .= ggw_state_reporting_msa_cell($total_customers, 7, STR_PAD_LEFT, '0');
	$report .= '  ';

	$purchases = ggw_state_reporting_msa_cell($total_purchases, 8, STR_PAD_LEFT, '0');
	$report .= ggw_state_reporting_msa_cell($purchases, 48);
	$report .= '001 ';

	$qty = ggw_state_reporting_msa_cell(number_format($total_qty, 2, '.', ''), 14, STR_PAD_LEFT, '0');
	$report .= ggw_state_reporting_msa_cell($qty, 32);
	$report .= '003 ';
	$report .= ggw_state_reporting_msa_cell(number_format($total_amount, 2, '.', ''), 14, STR_PAD_LEFT, '0');

	print $report;
}

function ggw_msa_generate_customer_report($start_date, $end_date, &$total_customers, &$total_purchases, &$total_amount, &$total_qty) {
	$report = '';

	$customers = db_query("SELECT uo.uid, uo.order_status, GROUP_CONCAT(DISTINCT uo.order_id ORDER BY uo.order_id ASC SEPARATOR ',') AS tickets FROM {uc_orders} uo
	 INNER JOIN {pos_api_expose_transaction_log} tl ON tl.ticket_id = uo.order_id
	 WHERE uo.order_status = 'pos_completed' OR uo.order_status = 'pos_return_closed' AND tl.timestamp >= '%s' AND tl.timestamp <= '%s'
	 GROUP BY uo.uid, uo.order_status", $start_date, $end_date);

	 $customers_displayed = array();
	 while($customer = db_fetch_object($customers)) {
	 	$user = user_load($customer->uid);
	 	$profile_node = db_fetch_object(db_query("SELECT n.nid, n.title FROM {node} n WHERE n.type = 'profile' AND n.uid = '%d'", $customer->uid));
 		$profile = db_fetch_object(db_query("SELECT * FROM content_type_profile ctp WHERE nid = '%d'", $profile_node->nid));

	 	$first_digit = $user->name;
	 	if(is_numeric($first_digit{0})) {
	 		$first_part = 'W'.substr($user->name, 0, 5);
	 		$second_part = 'W'.substr($user->name, 0, -5);
	 	} else {
	 		$first_part = $user->name;
	 		$second_part = $user->name;
	 	}

	 	$purchases = db_query("SELECT op.nid, SUM(op.qty) AS quantity, SUM(op.qty*op.price) AS price, op.model FROM uc_order_products op
	 		INNER JOIN term_node tn ON op.nid = tn.nid
	 		INNER JOIN ggw_state_reporting_terms sr ON sr.vid = tn.tid
	 	 WHERE op.order_id IN (%s) AND sr.setting > 1
	 	 GROUP BY op.nid", $customer->tickets);

	 	while($purchase = db_fetch_object($purchases)) {
	 		if(!isset($customers_displayed[$customer->uid])) {
	 			$total_customers++;
			 	$report .= 'SID';
			 	$report .= ggw_state_reporting_msa_cell($first_part, 8);
			 	$report .= ggw_state_reporting_msa_cell($second_part, 16);

			 	$report .= ggw_state_reporting_msa_cell($profile_node->title, 40);
			 	$report .= ggw_state_reporting_msa_cell($profile->field_company_address_street1.' '.$profile->field_company_address_street2, 90);
			 	$report .= ggw_state_reporting_msa_cell($profile->field_company_address_city, 25);
			 	$report .= ggw_state_reporting_msa_cell($profile->field_company_address_state.$profile->field_company_address_zip, 14);
			 	$report .= ggw_state_reporting_msa_cell($profile->field_company_address_state, 15);
			 	$report .= ggw_state_reporting_msa_cell('Retail', 27);
			 	$report .= ggw_state_reporting_msa_cell('N', 259);
			 	$report .= 'Y'."\n";

			 	$customers_displayed[$customer->uid] = true;
	 		}

	 		$report .= 'PUR';
	 		$report .= ggw_state_reporting_msa_cell($first_part, 8);
	 		$report .= ggw_state_reporting_msa_cell($second_part, 16);
	 		$report .= ggw_state_reporting_msa_cell(ggw_state_reporting_msa_cell($purchase->model, 14, STR_PAD_LEFT, '0'), 75);
	 		$report .= '001';
	 		$total_purchases++;

	 		//Positive Qty for closed ticket
	 		if($customer->order_status == 'pos_completed') {
	 			$report .= ' ';
	 		} else {
	 			$report .= '-';
	 		}

	 		$report .= ggw_state_reporting_msa_cell($purchase->quantity, 7, STR_PAD_LEFT, '0');
	 		$report .= '.00002';

	 		//Positive total for closed tickets
	 		if($customer->order_status == 'pos_completed') {
	 			$report .= ' ';
	 			$total_amount += $purchase->price;
	 			$total_qty += $purchase->quantity;
	 		} else {
	 			$report .= '-';
	 			$total_amount -= $purchase->price;
	 			$total_qty -= $purchase->quantity;
	 		}

	 		$report .= ggw_state_reporting_msa_cell(number_format($purchase->price, 2, '.', ''), 7, STR_PAD_LEFT, '0');
	 		$report .= "\n";
	 	}
	 }

	 return $report;
}

function ggw_state_reporting_msa_get_inventory_report(&$total_products) {
	$sql = "SELECT n.nid, n.title, up.model, ctp.field_prod_packing_value, ctp.field_msa_discount_remarks_value, ctp.field_prod_form_value, sr.vid as tid, sr.msa_cat FROM uc_products up
			INNER JOIN term_node tn ON up.nid = tn.nid
 			INNER JOIN node n ON up.nid = n.nid
 			INNER JOIN content_type_product ctp ON n.vid = ctp.vid
			INNER JOIN ggw_state_reporting_terms sr ON sr.vid = tn.tid
			WHERE setting > 1";

	$content = '';
	$bid = ggw_state_reporting_msa_cell('BID', 5);
	$products = db_query($sql);

	while($product = db_fetch_object($products)) {
		$total_products++;
		$content .= $bid.ggw_state_reporting_msa_cell($product->model, 12, STR_PAD_LEFT);
		$content .= '00'.ggw_state_reporting_msa_cell($product->model, 12, STR_PAD_LEFT, '0');
		
		if($product->field_prod_packing_value) {
			$content .= ggw_state_reporting_msa_cell(stripslashes($product->title), 26);
			$content .= ggw_state_reporting_msa_cell($product->field_prod_packing_value, 24);
		} else {
			$content .= ggw_state_reporting_msa_cell(stripslashes($product->title), 50);
		}
		
		$content .= ggw_state_reporting_msa_cell($product->field_msa_discount_remarks_value, 50);

		$content .= ggw_state_reporting_msa_cell($product->field_prod_form_value, 6, STR_PAD_LEFT, '0');

		if(empty($product->field_msa_discount_remarks_value)) {
			$content .= 'N';
			$discount_flag = '';
		} else {
			$content .= 'Y';
			$discount_flag = 'C';
		}

		$content .= ggw_state_reporting_msa_cell($product->tid, 2, STR_PAD_LEFT, '0');
		$content .= '    ';

		$msa_cat = ggw_state_reporting_msa_cell($product->msa_cat, 6, STR_PAD_LEFT, '0');
		$content .= ggw_state_reporting_msa_cell($msa_cat, 32);
		$content .= ggw_state_reporting_msa_cell($discount_flag, 71);

		$content .= '003';

		$status = pos_stock_management_status($product->nid);
		$quantity_on_hand = $status['quantity'];
		if($quantity_on_hand < 0) {
			$content .= '-';
		} else {
			$content .= ' ';
		}

		$padded_qty = number_format(abs($quantity_on_hand), 2, '.', '');
		$content .= ggw_state_reporting_msa_cell($padded_qty, 10, STR_PAD_LEFT, '0');

		$content .= "\n";
		$total_products++;
	}

	return $content;
}
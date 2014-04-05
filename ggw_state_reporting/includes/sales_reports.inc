<?php
function _ggw_pos_report_daily() {
	if($_GET['register_select']) {
		$register_select = $_GET['register_select'];
	} else {
		$register_select = false;
	}

	if($_GET['start_date']) {
		$start_date = strtotime($_GET['start_date']['date'].' '.$_GET['start_date']['time']);
	} else {
		$start_date = mktime(7, 0, 0);
	}

	if($_GET['end_date']) {
		$end_date = strtotime($_GET['end_date']['date'].' '.$_GET['end_date']['time']);
	} else {
		$end_date = mktime(22, 0, 0);
	}

	$content = drupal_get_form('ggw_state_reporting_date_range_form');

	$settlements = db_query("SELECT u.uid, u.credit_amount, s.settlement_amount, s.payment_type, s.remarks, s.settlement_date FROM user_term_credits_settlements s
							INNER JOIN user_term_credits_usages u ON s.cuid = u.cuid
							WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s'", $start_date, $end_date);

	$total_settlements = db_result(db_query("SELECT SUM(s.settlement_amount)
							FROM user_term_credits_settlements s
							WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s'", $start_date, $end_date));

	$transacs_count = db_result(db_query("SELECT COUNT(*)
							FROM user_term_credits_settlements s
							WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s'", $start_date, $end_date));

	$header = array('Date', 'Register', 'Customer', 'Payment', 'Payby', 'Amount');
	$rows = array();

	while($settlement = db_fetch_object($settlements)) {
		$rows[] = array(
			date('n/j/Y', $settlement->settlement_date),
			'',
			_ggw_state_reporting_cust_name($transac->uid),
			uc_currency_format($settlement->settlement_amount),
			$settlement->payment_type,
			uc_currency_format($settlement->credit_amount)
		);
	}

	$content .= theme('table', $header, $rows);

	$transacs = db_query("SELECT tl.credit_usage_id, tl.cash_pid, tl.rma_pid, tl.cc_pid, tl.check_pid, tl.mo_pid, tl.amount, tl.pid, tl.cuid, tl.uid, tl.timestamp, tl.register_id, tl.ticket_id, cl.amount_paid AS cash_paid, cl.change AS change_made, cl.total AS cash_total, chl.paid_amount AS check_payment, chl.post_dated AS check_post_dated, chl.cash_date AS check_cash_date, ccl.amount_paid AS cc_amount, mo.paid_amount AS mo_paid_amount, mo.reference AS mo_ref FROM {pos_api_expose_transaction_log} tl
	LEFT JOIN {pos_api_expose_cash_log} cl ON tl.cash_pid = cl.pid
	LEFT JOIN {pos_api_expose_credit_card_log} ccl ON tl.cc_pid = ccl.pid
	LEFT JOIN {pos_api_expose_check_log} chl ON tl.check_pid = chl.pid
	LEFT JOIN {pos_api_expose_money_order_log} mo ON tl.mo_pid = mo.pid
	WHERE tl.timestamp >= '%s' AND tl.timestamp <= '%s'", $start_date, $end_date);

	$header = array('Date', 'Register', 'Invoice', 'Customer', 'Total', 'Payby', 'Payment', 'Employee');
	$rows = array();

	//Stats
	while($transac = db_fetch_object($transacs)) {
		$payby = '';
		$payment = '';
		if($transac->cash_pid) {
			$payby .= 'CASH<br />';
			$payby .= 'CHANGE<br />';
			$payment .= uc_currency_format($transac->cash_paid).'<br />';
			$payment .= uc_currency_format($transac->change_made).'<br />';
		}

		if($transac->cc_pid) {
			$payby .= 'Credit Card - '.$transac->transaction_id.'<br />';
			$payment .= uc_currency_format($transac->cc_amount).'<br />';
		}

		if($transac->check_pid) {
			if($transac->check_post_dated) {
				$payby .= 'POST DATED CHECK'.'<br />';
				$payment .= uc_currency_format($transac->check_payment);
				$payment .= ' - '.$transac->check_cash_date.'<br />';
			} else {
				$payby .= 'CHECK'.'<br />';
				$payment .= uc_currency_format($transac->check_payment).'<br />';
			}
		}

		if($transac->mo_pid) {
			$payby .= 'Money Order - '.$transac->mo_ref.'<br />';
			$payment .= uc_currency_format($transac->mo_paid_amount).'<br />';
		}

		if($transac->credit_usage_id) {
			$payby .= 'AR'.'<br />';
			$payment .= '<br />';
		}

		if($transac->rma_pid) {
			$payby .= 'AR'.'<br />';
			$payment .= uc_currency_format($transac->amount).'-<br />';
		}

		$rows[] = array(
			date('n/j/Y', $transac->timestamp),
			$transac->register_id,
			$transac->pid,
			_ggw_state_reporting_cust_name($transac->cuid),
			uc_currency_format($transac->amount),
			$payby,
			$payment,
			_ggw_state_reporting_cust_name($transac->uid)
		);

		$transacs_count++;
	}

	$content .= theme('table', $header, $rows, array('class' => 'invoice-report'));

	$header = array('Total Registers', '', '', '');
	$rows = array();

	$cash_sales = db_result(db_query("SELECT SUM(cl.total) FROM pos_api_expose_cash_log cl WHERE cl.timestamp >= '%s' AND cl.timestamp <= '%s'", $start_date, $end_date));
	
	$check_sales = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_check_log cl WHERE cl.timestamp >= '%s' AND cl.timestamp <= '%s'", $start_date, $end_date));

	$check_sales_post = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_check_log cl WHERE post_dated = '1' AND cl.timestamp >= '%s' AND cl.timestamp <= '%s'", $start_date, $end_date));
	
	$cc_sales = db_result(db_query("SELECT SUM(amount_paid) FROM pos_api_expose_credit_card_log cc WHERE cc.timestamp >= '%s' AND cc.timestamp <= '%s'", $start_date, $end_date));
	$mo_sales = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_money_order_log mo WHERE mo.timestamp >= '%s' AND mo.timestamp <= '%s'", $start_date, $end_date));
	$ar_sales = db_result(db_query("SELECT SUM(amount) FROM pos_api_expose_transaction_log tl WHERE credit_usage_id > 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s'", $start_date, $end_date));

	$total_sales = $cash_sales+$check_sales+$cc_sales+$mo_sales+$ar_sales;

	$rma_returns = db_result(db_query("SELECT SUM(total_refund) FROM pos_api_expose_rma_refund_log rl WHERE rl.timestamp >= '%s' AND rl.timestamp <= '%s'", $start_date, $end_date));

	$product_cost = db_result(db_query("SELECT SUM(cost*qty) FROM uc_order_products uop
	INNER JOIN uc_orders uo ON uop.order_id = uo.order_id
	INNER JOIN pos_api_expose_transaction_log tl ON uo.order_id = tl.ticket_id
	WHERE uo.order_status = 'pos_completed' AND tl.timestamp >= '%s' AND tl.timestamp <= '%s'", $start_date, $end_date));

	$total_invoice = db_result(db_query("SELECT SUM(uo.order_total) FROM uc_orders uo
	 INNER JOIN pos_api_expose_transaction_log tl ON uo.order_id = tl.ticket_id
	 WHERE uo.order_status = 'pos_completed' AND tl.timestamp >= '%s' AND tl.timestamp <= '%s'", $start_date, $end_date));

	$sales_tax = db_result(db_query("SELECT SUM(li.amount) FROM uc_order_line_items li
	INNER JOIN uc_orders uo ON li.order_id = uo.order_id
	INNER JOIN pos_api_expose_transaction_log tl ON uo.order_id = tl.ticket_id
	WHERE uo.order_status = 'pos_completed' AND li.type = 'tax' AND tl.timestamp >= '%s' AND tl.timestamp <= '%s'", $start_date, $end_date));

	$discount = 0; //This is not manager overrides, but coupons and rma discounts.
	$coupons = 0;

	$net_sale = $total_sales-($rma_returns+$sales_tax);
	$spacing = '&nbsp;&nbsp;&nbsp;';
	$rows[] = array(
		$spacing.'Sales',
		uc_currency_format($total_sales),
		$spacing.'Net Sales',
		uc_currency_format($net_sale)
	);

	$rows[] = array(
		'- Returns',
		uc_currency_format($rma_returns),
		'- Coupons',
		uc_currency_format($coupons)
	);

	$rows[] = array(
		'- Discount',
		uc_currency_format($discount),
		'+ Sales Tax',
		uc_currency_format($sales_tax)
	);

	$rows[] = array(
		$spacing.'Net Sales',
		uc_currency_format($net_sale),
		$spacing.'Total Invoice Amount',
		uc_currency_format($total_invoice)
	);

	$rows[] = array(
		'- Product Cost',
		uc_currency_format($product_cost),
		$spacing.'Total Customer Deposit',
		uc_currency_format($total_settlements)
	);

	$rows[] = array(
		$spacing.'Gross Profit',
		uc_currency_format($net_sale-$product_cost),
		$spacing.'Total Credit Sales',
		uc_currency_format($ar_sales)
	);

	if($net_sale == 0) {
		$margin = 0;
	} else {
		$margin = ($net_sale-$product_cost)/$net_sale;
	}

	$rows[] = array(
		$spacing.'G/P Margin Ratio',
		number_format($margin, 2).'%',
		$spacing.'Total Transaction Count',
		$transacs_count
	);

	$content .= theme('table', $header, $rows);


	$header = array('Payment', 'Total');
	$rows = array();
	$rows[] = array(
		'Cash',
		uc_currency_format($cash_sales)
	);

	$rows[] = array(
		'Check',
		uc_currency_format($check_sales)
	);

	$rows[] = array(
		'Money Order',
		uc_currency_format($mo_sales)
	);

	$rows[] = array(
		'Credit Card',
		uc_currency_format($cc_sales)
	);

	$rows[] = array(
		'Post Dated Check',
		uc_currency_format($check_sales_post)
	);

	$content .= theme('table', $header, $rows);
	return $content;
}

function _ggw_state_reporting_cust_name($uid) {
	$user = db_result(db_query("SELECT n.title FROM node n WHERE n.uid = '".$uid."' AND n.type = 'profile'"));
	if($user) {
		return l($user, 'user/'.$uid);
	} else {
		 $username = db_result(db_query("SELECT name FROM {users} WHERE uid = '%s'", $uid));
		 return l($username, 'user/'.$uid);
	}
}
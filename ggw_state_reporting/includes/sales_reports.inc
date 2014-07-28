
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

	drupal_set_title('Daily Sales Report - ('.date('n/j/Y h:i a', $start_date).' - '.date('n/j/Y h:i a', $end_date).')');

	$content = drupal_get_form('ggw_state_reporting_date_range_form');

	/*$settlements = db_query("SELECT u.uid, u.credit_amount, s.settlement_amount, s.payment_type, s.remarks, s.settlement_date, s.euid, s.register_id FROM user_term_credits_settlements s
							INNER JOIN user_term_credits_usages u ON s.cuid = u.cuid
							WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s'", $start_date, $end_date);*/

	$settlements = db_query("SELECT u.uid, SUM(s.settlement_amount) AS settlement_amount, s.payment_type, s.remarks, s.settlement_date, s.euid, s.register_id FROM user_term_credits_settlements s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' GROUP BY u.uid, s.settlement_date ORDER BY s.settlement_date ASC", $start_date, $end_date);

	$total_settlements = db_result(db_query("SELECT SUM(s.settlement_amount)
							FROM user_term_credits_settlements s
							WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s'", $start_date, $end_date));

	$transacs_count = db_result(db_query("SELECT COUNT(*)
							FROM user_term_credits_settlements s
							WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s'", $start_date, $end_date));

	$header = array('Date', 'Register', 'Customer', 'Payment', 'Payby', 'Amount', 'Employee');
	$payment_types = array(
		'pos_payment' => 'Register Payment',
		'cash' => 'Cash - B.O.',
		'check' => 'Check - B.O.',
		'money_order' => 'M.O. - B.O.',
		'post_check' => 'Post Dated Check - B.O.'
	);
	$rows = array();
	$settlement_close = array();
	while($settlement = db_fetch_object($settlements)) {
		if($settlement->payment_type == 'pos_payment') {
			$settlement_components = explode("\n", $settlement->remarks);
			foreach($settlement_components as $settlement_component) {
				$component = split('\|', $settlement_component);
				if(!empty($component[0])) {
					if(!array_key_exists($component[0], $settlement_close)) {
						$settlement_close[$component[0]] = 0;
					}
					$settlement_close[$component[0]] += (int)(((double)$component[1])*100);
				}
			}
		} else if($settlement->payment_type == 'cash') {
			$component = 'CASH';
			if(!array_key_exists($component, $settlement_close)) {
				$settlement_close[$component] = 0;
			}
			$settlement_close[$component] += (int)(((double)$settlement->settlement_amount)*100);
		} else if($settlement->payment_type == 'check') {
			$component = 'CHECK';
			if(!array_key_exists($component, $settlement_close)) {
				$settlement_close[$component] = 0;
			}
			$settlement_close[$component] += (int)(((double)$settlement->settlement_amount)*100);
		} else if($settlement->payment_type == 'money_order') {
			$component = 'MO';
			if(!array_key_exists($component, $settlement_close)) {
				$settlement_close[$component] = 0;
			}
			$settlement_close[$component] += (int)(((double)$settlement->settlement_amount)*100);
		} else if($settlement->payment_type == 'post_check') {
			$component = 'PCHECK';
			if(!array_key_exists($component, $settlement_close)) {
				$settlement_close[$component] = 0;
			}
			$settlement_close[$component] += (int)(((double)$settlement->settlement_amount)*100);
		}
		$column_1 = '';
		$column_2 = '';
		$pay_items = split("\n", $settlement->remarks);
		$i = 0;
		foreach($pay_items as $pay_item) {
			$components = split("\|", $pay_item);
			$column_1 .= $components[0];
			$column_2 .= uc_currency_format($components[1], false);
			if($i != count($pay_items)-1) {
				$column_1 .= '<br />';
				$column_2 .= '<br />';
			}
			$i++;
		}

		$rows[] = array(
			date('n/j/Y', $settlement->settlement_date),
			$settlement->register_id,
			_ggw_state_reporting_cust_name($settlement->uid),
			uc_currency_format($settlement->settlement_amount, false),
			//$settlement->remarks,
			//uc_currency_format($settlement->credit_amount),
			$column_1,
			$column_2,
			_ggw_state_reporting_cust_name($settlement->euid)
		);
	}

	$content .= theme('table', $header, $rows);

	$transacs = db_query("SELECT tl.credit_usage_id, tl.cash_pid, tl.rma_pid, tl.cc_pid, tl.check_pid, tl.mo_pid, tl.amount, tl.pid, tl.cuid, tl.uid, tl.timestamp, tl.register_id, tl.ticket_id, cl.amount_paid AS cash_paid, cl.change AS change_made, cl.total AS cash_total, chl.paid_amount AS check_payment, chl.check_number, chl.post_dated AS check_post_dated, chl.cash_date AS check_cash_date, ccl.amount_paid AS cc_amount, mo.paid_amount AS mo_paid_amount, mo.reference AS mo_ref, ccl.transaction_id FROM {pos_api_expose_transaction_log} tl
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
			if($transac->cash_paid > 0 || $transac->change_made > 0) {
				$payby .= 'CASH<br />';
				$payby .= 'CHANGE<br />';
				$payment .= uc_currency_format($transac->cash_paid, false).'<br />';
				$payment .= uc_currency_format($transac->change_made, false).'<br />';
			}
		}

		if($transac->cc_pid) {
			$payby .= 'Credit Card';
			if($transac->transaction_id) {
				$payby .= ' - '.$transac->transaction_id;
			}
			$payby .= '<br />';
			$payment .= uc_currency_format($transac->cc_amount, false).'<br />';
		}

		if($transac->check_pid) {
			if($transac->check_post_dated) {
				$payby .= 'POST DATED CHECK';
				if($transac->check_number) {
					$payby .= ' - '.$transac->check_number;
				}
				$payby .= '<br />';
				$payment .= uc_currency_format($transac->check_payment, false);
				$payment .= ' - '.$transac->check_cash_date.'<br />';
			} else {
				$payby .= 'CHECK';
				if($transac->check_number) {
					$payby .= ' - '.$transac->check_number.'<br />';
				} else {
					$payby .= '<br />';
				}

				$payment .= uc_currency_format($transac->check_payment, false);
				$payment .= '<br />';
			}
		}

		if($transac->mo_pid) {
			if($transac->mo_ref) {
				$payby .= 'Money Order - '.$transac->mo_ref.'<br />';
			} else {
				$payby .= 'Money Order'.'<br />';
			}
			$payment .= uc_currency_format($transac->mo_paid_amount, false).'<br />';
		}

		if($transac->credit_usage_id) {
			$payby .= 'AR'.'<br />';
			$payment .= '<br />';
		}

		if($transac->rma_pid) {
			$payby .= 'RMA CREDIT'.'<br />';
			$rma = db_result(db_query("SELECT total_refund FROM pos_api_expose_rma_refund_log WHERE pid = '%d'", $transac->rma_pid));
			$payment .= uc_currency_format((-1)*$rma, false).'<br />';
		}

		$rows[] = array(
			date('n/j/Y', $transac->timestamp),
			$transac->register_id,
			$transac->pid,
			_ggw_state_reporting_cust_name($transac->cuid),
			uc_currency_format($transac->amount, false),
			$payby,
			$payment,
			_ggw_state_reporting_cust_name($transac->uid)
		);

		$transacs_count++;
	}

	$content .= theme('table', $header, $rows, array('class' => 'invoice-report'));

	$header = array('Total Registers', '', '', '');
	$rows = array();

	$cash_sales = db_result(db_query("SELECT SUM(cl.total) FROM pos_api_expose_cash_log cl INNER JOIN pos_api_expose_transaction_log tl ON tl.cash_pid = cl.pid WHERE tl.rma_pid = '0' AND tl.cc_pid = '0' AND tl.check_pid = '0' AND tl.mo_pid = '0' AND cl.timestamp >= '%s' AND cl.timestamp <= '%s'", $start_date, $end_date));
	
	$check_sales = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_check_log cl WHERE post_dated = '0' AND cl.timestamp >= '%s' AND cl.timestamp <= '%s'", $start_date, $end_date));

	$check_sales_post = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_check_log cl WHERE post_dated = '1' AND cl.timestamp >= '%s' AND cl.timestamp <= '%s'", $start_date, $end_date));
	
	$cc_sales = db_result(db_query("SELECT SUM(amount_paid) FROM pos_api_expose_credit_card_log cc WHERE cc.timestamp >= '%s' AND cc.timestamp <= '%s'", $start_date, $end_date));
	$mo_sales = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_money_order_log mo WHERE mo.timestamp >= '%s' AND mo.timestamp <= '%s'", $start_date, $end_date));
	$ar_sales = db_result(db_query("SELECT SUM(amount) FROM pos_api_expose_transaction_log tl WHERE credit_usage_id > 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s'", $start_date, $end_date));

	$total_sales = $cash_sales+$check_sales+$cc_sales+$mo_sales+$ar_sales+$check_sales_post;

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
		uc_currency_format($total_sales, false),
		$spacing.'Net Sales',
		uc_currency_format($net_sale, false)
	);

	$rows[] = array(
		'-&nbsp;&nbsp;Returns',
		uc_currency_format($rma_returns, false),
		'-&nbsp;&nbsp;Coupons',
		uc_currency_format($coupons, false)
	);

	$rows[] = array(
		'-&nbsp;&nbsp;Discount',
		uc_currency_format($discount, false),
		'+ Sales Tax',
		uc_currency_format($sales_tax, false)
	);

	$rows[] = array(
		$spacing.'Net Sales',
		uc_currency_format($net_sale, false),
		$spacing.'Total Invoice Amount',
		uc_currency_format($total_invoice, false)
	);

	$rows[] = array(
		'-&nbsp;&nbsp;Product Cost',
		uc_currency_format($product_cost, false),
		$spacing.'Total Customer Deposit',
		uc_currency_format($total_settlements, false)
	);

	$rows[] = array(
		$spacing.'Gross Profit',
		uc_currency_format($net_sale-$product_cost, false),
		$spacing.'Total Credit Sales',
		uc_currency_format($ar_sales, false)
	);

	if($net_sale == 0) {
		$margin = 0;
	} else {
		$margin = ($net_sale-$product_cost)/$net_sale;
	}

	$rows[] = array(
		$spacing.'G/P Margin Ratio',
		number_format($margin*100, 2).'%',
		$spacing.'Total Transaction Count',
		$transacs_count
	);

	$content .= theme('table', $header, $rows);


	$header = array('Payment', 'Total');
	$rows = array();

	$cash_sales += (($settlement_close['CASH']-$settlement_close['CHANGE'])/100);
	$rows[] = array(
		'Cash',
		uc_currency_format($cash_sales, false)
	);

	$check_sales += $settlement_close['CHECK']/100;
	$rows[] = array(
		'Check',
		uc_currency_format($check_sales, false)
	);

	$mo_sales += $settlement_close['MO']/100;
	$rows[] = array(
		'Money Order',
		uc_currency_format($mo_sales, false)
	);

	$cc_sales += $settlement_close['CC']/100;
	$rows[] = array(
		'Credit Card',
		uc_currency_format($cc_sales, false)
	);

	$check_sales_post += $settlement_close['PCHECK']/100;
	$rows[] = array(
		'Post Dated Check',
		uc_currency_format($check_sales_post, false)
	);

	$content .= theme('table', $header, $rows);
	return $content;
}
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

	$dp = (bool)$_GET['dp'];

	drupal_set_title('Daily Sales Report - ('.date('n/j/Y h:i a', $start_date).' - '.date('n/j/Y h:i a', $end_date).')');

	$content = drupal_get_form('ggw_state_reporting_date_range_form');

	/*$settlements = db_query("SELECT u.uid, u.credit_amount, s.settlement_amount, s.payment_type, s.remarks, s.settlement_date, s.euid, s.register_id FROM user_term_credits_settlements s
							INNER JOIN user_term_credits_usages u ON s.cuid = u.cuid
							WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s'", $start_date, $end_date);*/

	if(!$dp) {
		if($register_select) {
			$settlement_sql_inject = "AND s.register_id = '".$register_select."'";
			$transaction_sql_inject = "AND tl.register_id = '".$register_select."'";
		} else {
			$settlement_sql_inject = "";
			$transaction_sql_inject = "";
		}

		$settlements_sql = "SELECT s.sid, u.uid, SUM(s.settlement_amount) AS settlement_amount, s.payment_type, s.remarks, s.settlement_date, s.euid, s.register_id FROM (SELECT * FROM user_term_credits_settlements si ORDER BY si.sid ASC) s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' ".$settlement_sql_inject." GROUP BY u.uid, s.settlement_date ORDER BY s.settlement_date ASC";

		$settlements = db_query($settlements_sql, $start_date, $end_date);

		$total_settlements = db_result(db_query("SELECT SUM(s.settlement_amount)
								FROM user_term_credits_settlements s
								WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' ".$settlement_sql_inject, $start_date, $end_date));

		$transacs_count = 0;
		$stashed_rma_change = '0';

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
			$transacs_count++;
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

			$payment_types_cr = array(
				'MO' => 'MONEY ORDER',
				'CC' => 'CREDIT CARD',
				'PCHECK' => 'POST DATED CHECK'
			);

			foreach($pay_items as $pay_item) {
				$components = split("\|", $pay_item);
				if($components[0] == 'CASH' || $components[0] == 'CHANGE') {
					$stash = db_fetch_object(db_query("SELECT `change`, stashed FROM user_term_credits_cash_log WHERE sid = '%d'", $settlement->sid));
					if($stash->stashed) {	
						if($components[0] == 'CHANGE') {
							$stashed_rma_change = bcadd($stash->change, $stashed_rma_change, 6);
							$column_1 .= 'RMA SENT ';
						}
					}

					$column_1 .= $components[0];
					$column_2 .= uc_currency_format(user_term_credits_recursive_round($components[1]), false);
					$column_1 .= '<br />';
					$column_2 .= '<br />';
				}
			}

			$i = 0;
			foreach($pay_items as $pay_item) {
				$components = split("\|", $pay_item);
				if($components[0] != 'CASH' && $components[0] != 'CHANGE' && $components[1] > 0) {
					if($payment_types_cr[$components[0]]) {
						$column_1 .= $payment_types_cr[$components[0]];
					} else {
						$column_1 .= $components[0];
					}
					if($components[0] == 'CHECK' || $components[0] == 'PCHECK') {
						$check_info = db_fetch_object(db_query("SELECT check_number, cash_date, post_dated FROM user_term_credits_check_log WHERE sid = '%d'", $settlement->sid));
						if($check_info->check_number) {
							$column_1 .= ' - '.$check_info->check_number;
						}
						if($check_info->post_dated) {
							if($check_info->cash_date) {
								$column_1 .= ' - '.$check_info->cash_date;
							}
						}
					}
					if($components[0] == 'CC') {
						$transaction_id = db_result(db_query("SELECT transaction_id FROM user_term_credits_credit_card_log WHERE sid = '%d'", $settlement->sid));
						if($transaction_id) {
							$column_1 .= ' - '.$transaction_id;
						}
					}
					if($components[0] == 'MO') {
						$mo_payment = db_result(db_query("SELECT reference FROM user_term_credits_money_order_log WHERE sid = '%d'", $settlement->sid));
						if($mo_payment) {
							$column_1 .= ' - '.$mo_payment;
						}
					}
					$column_2 .= uc_currency_format(user_term_credits_recursive_round($components[1]), false);
					if($i != count($pay_items)-1) {
						$column_1 .= '<br />';
						$column_2 .= '<br />';
					}
				}
				$i++;
			}

			if(user_access('edit previous payments')) {
				$column_1 .= l('Delete Payment', 'admin/store/reports/pos-sales/remove-sid/'.$settlement->sid);
			}

			$rows[] = array(
				date('n/j/Y', $settlement->settlement_date),
				$settlement->register_id,
				_ggw_state_reporting_cust_name($settlement->uid),
				uc_currency_format(user_term_credits_recursive_round($settlement->settlement_amount), false),
				//$settlement->remarks,
				//uc_currency_format($settlement->credit_amount),
				$column_1,
				$column_2,
				_ggw_state_reporting_cust_name($settlement->euid)
			);
		}

		$content .= theme('table', $header, $rows);
	}

	if($dp) {
		$content .= drupal_get_form('pos_api_expose_clear_dp_form');
	}

	if($dp) {
		$cash_log_table = 'pos_api_expose_cash_log_dp';
		$check_log_table = 'pos_api_expose_check_log_dp';
		$mo_log_table = 'pos_api_expose_money_order_log_dp';
		$cc_log_table = 'pos_api_expose_credit_card_log_dp';
		$transac_log_table = 'pos_api_expose_transaction_log_dp';
	} else {
		$cash_log_table = 'pos_api_expose_cash_log';
		$check_log_table = 'pos_api_expose_check_log';
		$mo_log_table = 'pos_api_expose_money_order_log';
		$cc_log_table = 'pos_api_expose_credit_card_log';
		$transac_log_table = 'pos_api_expose_transaction_log';
	}

	$transacs = db_query("SELECT tl.credit_usage_id, tl.cash_pid, tl.rma_pid, tl.cc_pid, tl.check_pid, tl.mo_pid, tl.amount, tl.pid, tl.cuid, tl.uid, tl.timestamp, tl.register_id, tl.ticket_id, cl.amount_paid AS cash_paid, cl.change AS change_made, cl.total AS cash_total, chl.paid_amount AS check_payment, chl.check_number, chl.post_dated AS check_post_dated, chl.cash_date AS check_cash_date, ccl.amount_paid AS cc_amount, mo.paid_amount AS mo_paid_amount, mo.reference AS mo_ref, cu.credit_amount AS ar_payment_amount, ccl.transaction_id FROM {".$transac_log_table."} tl
	LEFT JOIN {".$cash_log_table."} cl ON tl.cash_pid = cl.pid
	LEFT JOIN {".$cc_log_table."} ccl ON tl.cc_pid = ccl.pid
	LEFT JOIN {".$check_log_table."} chl ON tl.check_pid = chl.pid
	LEFT JOIN {".$mo_log_table."} mo ON tl.mo_pid = mo.pid
	LEFT JOIN {user_term_credits_usages} cu ON tl.credit_usage_id = cu.cuid
	WHERE tl.timestamp >= '%s' AND tl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date);

	$header = array('Date', 'Register', 'Invoice', 'Customer', 'Total', 'Payby', 'Payment', 'Transaction Type', 'Ticket #', 'Employee');
	$rows = array();
	$total_transacs = 0;
	$rma_cash_out = 0;
	//Stats
	while($transac = db_fetch_object($transacs)) {
		$payby = '';
		$payment = '';
		if($transac->cash_pid) {
			if($transac->cash_paid > 0 || $transac->change_made > 0) {
				$payby .= 'CASH<br />';
				$payby .= 'CHANGE<br />';
				$payment .= uc_currency_format(user_term_credits_recursive_round($transac->cash_paid), false).'<br />';
				$payment .= uc_currency_format(user_term_credits_recursive_round($transac->change_made), false).'<br />';
			}
		}

		if($transac->cc_pid) {
			$payby .= 'Credit Card';
			if($transac->transaction_id) {
				$payby .= ' - '.$transac->transaction_id;
			}
			$payby .= '<br />';
			$payment .= uc_currency_format(user_term_credits_recursive_round($transac->cc_amount), false).'<br />';
		}

		if($transac->check_pid) {
			if($transac->check_post_dated) {
				$payby .= 'POST DATED CHECK';
				if($transac->check_number) {
					$payby .= ' - '.$transac->check_number;
				}
				$payby .= '<br />';
				$payment .= uc_currency_format(user_term_credits_recursive_round($transac->check_payment), false);
				$payment .= ' - '.$transac->check_cash_date.'<br />';
			} else {
				$payby .= 'CHECK';
				if($transac->check_number) {
					$payby .= ' - '.$transac->check_number.'<br />';
				} else {
					$payby .= '<br />';
				}

				$payment .= uc_currency_format(user_term_credits_recursive_round($transac->check_payment), false);
				$payment .= '<br />';
			}
		}

		if($transac->mo_pid) {
			if($transac->mo_ref) {
				$payby .= 'Money Order - '.$transac->mo_ref.'<br />';
			} else {
				$payby .= 'Money Order'.'<br />';
			}
			$payment .= uc_currency_format(user_term_credits_recursive_round($transac->mo_paid_amount), false).'<br />';
		}

		if($transac->credit_usage_id) {
			$payby .= 'AR'.'<br />';
			$payment .= uc_currency_format(user_term_credits_recursive_round($transac->ar_payment_amount), false).'<br />';
		}

		$ticket_type = 'SALE';
		if($transac->rma_pid < 0) {
			$payby .= 'RMA CASH OUT'.'<br />';
			$rma_cash_out += $transac->amount;
			$transac->amount = (-1)*$transac->amount;
			$payment .= uc_currency_format(user_term_credits_recursive_round($transac->amount), false);
		} else if($transac->rma_pid > 0) {
			$rma = db_result(db_query("SELECT (-1)*total_refund FROM pos_api_expose_rma_refund_log WHERE pid = '%d'", $transac->rma_pid));
			if($rma > 0) {
				$ticket_type = 'SALE w/ RMA';
				$payby .= 'RMA DEBIT'.'<br />';
			} else {
				$ticket_type = 'RMA';
				$payby .= 'RMA CREDIT'.'<br />';
				$transac->ticket_id = db_result(db_query("SELECT rma_ticket_id FROM {pos_api_expose_rma_refund_log} WHERE pid = '%d'", $transac->rma_pid));
			}
			$payment .= uc_currency_format(user_term_credits_recursive_round($rma), false).'<br />';
			if($rma < 0) {
				$transac->amount = (-1)*$transac->amount;
			}
		}

		if($transac->amount > 0) {
			$total_transacs += $transac->amount;
		}

		$rows[] = array(
			date('n/j/Y', $transac->timestamp),
			$transac->register_id,
			$transac->pid,
			_ggw_state_reporting_cust_name($transac->cuid),
			uc_currency_format(user_term_credits_recursive_round($transac->amount), false),
			$payby,
			$payment,
			$ticket_type,
			l($transac->ticket_id, 'admin/store/orders/'.$transac->ticket_id, array('attributes' => array('target' => '_blank'))),
			_ggw_state_reporting_cust_name($transac->uid)
		);

		$transacs_count++;
	}

	if(!$dp) {
		$cash_sales = db_result(db_query("SELECT SUM(cl.amount_paid) FROM pos_api_expose_cash_log cl INNER JOIN pos_api_expose_transaction_log tl ON tl.cash_pid = cl.pid WHERE cl.timestamp >= '%s' AND cl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));

		$cash_change = db_result(db_query("SELECT SUM(cl.change) FROM pos_api_expose_cash_log cl INNER JOIN pos_api_expose_transaction_log tl ON tl.cash_pid = cl.pid WHERE cl.timestamp >= '%s' AND cl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));
		
		if($register_select) {
			$check_sales = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_check_log cl
				INNER JOIN pos_api_expose_transaction_log tl ON tl.check_pid = cl.pid
				WHERE post_dated = '0' AND cl.timestamp >= '%s' AND cl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));

			$check_sales_post = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_check_log cl
				INNER JOIN pos_api_expose_transaction_log tl ON tl.check_pid = cl.pid
				WHERE post_dated = '1' AND cl.timestamp >= '%s' AND cl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));
			
			$cc_sales = db_result(db_query("SELECT SUM(amount_paid) FROM pos_api_expose_credit_card_log cc
				INNER JOIN pos_api_expose_transaction_log tl ON tl.cc_pid = cc.pid
				WHERE cc.timestamp >= '%s' AND cc.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));

			$mo_sales = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_money_order_log mo
				INNER JOIN pos_api_expose_transaction_log tl ON tl.mo_pid = mo.pid
				WHERE mo.timestamp >= '%s' AND mo.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));

			$ar_sales = db_result(db_query("SELECT SUM(cu.credit_amount) FROM pos_api_expose_transaction_log tl
				INNER JOIN {user_term_credits_usages} cu ON cu.cuid = tl.credit_usage_id
				WHERE credit_usage_id > 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));

			$rma_sales = db_result(db_query("SELECT (-1)*SUM(total_refund) FROM pos_api_expose_rma_refund_log rl
				INNER JOIN pos_api_expose_transaction_log tl ON tl.rma_pid = rl.pid
				WHERE rl.timestamp >= '%s' AND rl.timestamp <= '%s' AND rl.total_refund < 0 AND rl.pid NOT IN (SELECT crl.rma_pid FROM user_term_credits_rma_log crl) ".$transaction_sql_inject, $start_date, $end_date));
		} else {
			$check_sales = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_check_log cl WHERE post_dated = '0' AND cl.timestamp >= '%s' AND cl.timestamp <= '%s'", $start_date, $end_date));

			$check_sales_post = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_check_log cl WHERE post_dated = '1' AND cl.timestamp >= '%s' AND cl.timestamp <= '%s'", $start_date, $end_date));
			
			$cc_sales = db_result(db_query("SELECT SUM(amount_paid) FROM pos_api_expose_credit_card_log cc WHERE cc.timestamp >= '%s' AND cc.timestamp <= '%s'", $start_date, $end_date));
			$mo_sales = db_result(db_query("SELECT SUM(paid_amount) FROM pos_api_expose_money_order_log mo WHERE mo.timestamp >= '%s' AND mo.timestamp <= '%s'", $start_date, $end_date));
			$ar_sales = db_result(db_query("SELECT SUM(cu.credit_amount) FROM pos_api_expose_transaction_log tl
				INNER JOIN {user_term_credits_usages} cu ON cu.cuid = tl.credit_usage_id
				WHERE credit_usage_id > 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s'", $start_date, $end_date));

			$rma_sales = db_result(db_query("SELECT (-1)*SUM(total_refund) FROM pos_api_expose_rma_refund_log rl WHERE rl.timestamp >= '%s' AND rl.timestamp <= '%s' AND total_refund < 0 AND rl.pid NOT IN (SELECT crl.rma_pid FROM user_term_credits_rma_log crl)", $start_date, $end_date));
		}

		$total_sales = bcadd($cash_sales, $check_sales, 6);
		$total_sales = bcsub($total_sales, $cash_change, 6);
		$total_sales = bcadd($total_sales, $cc_sales, 6);
		$total_sales = bcadd($total_sales, $mo_sales, 6);
		$total_sales = bcadd($total_sales, $ar_sales, 6);
		$total_sales = bcadd($total_sales, $check_sales_post, 6);
		$total_sales = bcadd($total_sales, $rma_sales, 2);

		$error = bcsub($total_sales, $total_transacs, 6);
		$positivity = bccomp($error, "0", 6);
		if($positivity == -1) {
			$error = bcmul("-1", $error, 6);
		}

		$error_range = bccomp($error, "0.2", 6);
		$no_error = bccomp($error, "0", 6);
		if($no_error != 0 && $error_range == -1) {
			$rows[] = array(
				'',
				'',
				'',
				'Invoice Total',
				uc_currency_format($total_sales, false).' (Rounding Error: '.$error.')',
				'',
				'',
				'',
				'',
				''
			);
		} elseif($no_error == 0) {
			$rows[] = array(
				'',
				'',
				'',
				'Invoice Total',
				uc_currency_format($total_transacs, false),
				'',
				'',
				'',
				'',
				''
			);
		} else {
			$rows[] = array(
				'',
				'',
				'',
				'Invoice Total',
				uc_currency_format($total_transacs, false).' (Error: '.$error.')',
				'',
				'',
				'',
				'',
				''
			);
		}
	}

	$content .= theme('table', $header, $rows, array('class' => 'invoice-report'));

	if(!$dp) {
		$header = array('Total Registers', '', '', '');
		$rows = array();

		$rma_returns = db_result(db_query("SELECT SUM(rl.total_refund) FROM pos_api_expose_rma_refund_log rl
			INNER JOIN pos_api_expose_transaction_log tl ON tl.rma_pid = rl.pid
			WHERE rl.timestamp >= '%s' AND rl.timestamp <= '%s' AND rl.total_refund > 0 ".$transaction_sql_inject, $start_date, $end_date))+$rma_cash_out;

		$product_cost = db_result(db_query("SELECT SUM(cost*qty) FROM uc_order_products uop
		INNER JOIN uc_orders uo ON uop.order_id = uo.order_id
		INNER JOIN pos_api_expose_transaction_log tl ON uo.order_id = tl.ticket_id
		WHERE uo.order_status = 'pos_completed' AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));

		$total_invoice = db_result(db_query("SELECT SUM(uo.order_total) FROM uc_orders uo
		 INNER JOIN pos_api_expose_transaction_log tl ON uo.order_id = tl.ticket_id
		 WHERE uo.order_status = 'pos_completed' AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));

		$sales_tax = db_result(db_query("SELECT SUM(li.amount) FROM uc_order_line_items li
		INNER JOIN uc_orders uo ON li.order_id = uo.order_id
		INNER JOIN pos_api_expose_transaction_log tl ON uo.order_id = tl.ticket_id
		WHERE uo.order_status = 'pos_completed' AND li.type = 'tax' AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' ".$transaction_sql_inject, $start_date, $end_date));

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
		$cash_sales -= $cash_change;
		$cash_sales -= $rma_cash_out;
		$cash_sales += $stashed_rma_change;

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

		$total_payments = "0";
		$total_payments = bcadd($total_payments, $check_sales, 6);
		$total_payments = bcadd($total_payments, $cash_sales, 6);
		$total_payments = bcadd($total_payments, $mo_sales, 6);
		$total_payments = bcadd($total_payments, $cc_sales, 6);
		$total_payments = bcadd($total_payments, $check_sales_post, 6);

		$rows[] = array(
			'Total Payments',
			uc_currency_format($total_payments, false)
		);

		$content .= theme('table', $header, $rows);
	}

	if($dp) {
		$header = array('Total', '');
		$rows = array(array('Total DP Revenue', uc_currency_format($total_transacs)));
		$content .= theme('table', $header, $rows);
	}

	return $content;
}

function pos_api_expose_clear_dp_form() {
	$form = array();

	$form['remove_dp'] = array(
		'#type' => 'submit',
		'#value' => 'Clear DP Tickets',
		'#attributes' => array('style'=> 'margin: 10px 0;', 'onclick' => 'return confirm("Are you sure you want to continue with removal of dp tickets?")')
	);

	return $form;
}

function pos_api_expose_clear_dp_form_submit($form_state) {
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

	$tickets = db_query("SELECT tl.ticket_id FROM {pos_api_expose_transaction_log_dp} tl WHERE tl.timestamp >= '%s' AND tl.timestamp <= '%s'", $start_date, $end_date);
	while($ticket = db_fetch_object($tickets)) {
		uc_order_delete($ticket->ticket_id);
		$transacs = db_query("SELECT tl.pid, tl.cash_pid, tl.cc_pid, tl.check_pid, tl.mo_pid FROM {pos_api_expose_transaction_log_dp} tl WHERE tl.ticket_id = '%d'", $ticket->ticket_id);

		while($transac = db_fetch_object($transacs)) {
			db_query("DELETE FROM {pos_api_expose_cash_log_dp} WHERE pid = '%d'", $transac->cash_pid);
			db_query("DELETE FROM {pos_api_expose_credit_card_log_dp} WHERE pid = '%d'", $transac->cc_pid);
			db_query("DELETE FROM {pos_api_expose_check_log_dp} WHERE pid = '%d'", $transac->check_pid);
			db_query("DELETE FROM {pos_api_expose_money_order_log_dp} WHERE pid = '%d'", $transac->mo_pid);
			db_query("DELETE FROM {pos_api_expose_transaction_log_dp} WHERE pid = '%d'", $transac->pid);
		}
	}

	drupal_set_message("DP tickets were successfully removed.");
}

function pos_api_expose_remove_payment_confirm($sid) {
	return drupal_get_form('pos_api_expose_remove_payment_confirm_form', $sid);	
}

function pos_api_expose_remove_payment_confirm_form($form_state, $sid) {
	$form = array();
	$uri = referer_uri();
	$form['return_uri'] = array('#type' => 'value', '#value' => $uri);
	$form['payment_sid'] = array('#type' => 'value', '#value' => $sid);
	return confirm_form($form, 'Are you sure want to delete this payment?', $uri);
}

function pos_api_expose_remove_payment_confirm_form_submit($form, &$form_state) {
	drupal_set_message('Payment Deleted');
	pos_api_expose_remove_payments($form_state['values']['payment_sid']);
	drupal_goto($form_state['values']['return_uri']);
}

function pos_api_expose_remove_payments($sid) {
	$cuid = db_fetch_object(db_query("SELECT cs.settlement_date, cs.euid, cs.register_id FROM user_term_credits_settlements cs WHERE cs.sid = '%d'", $sid));
	$cuids = db_query("SELECT cs.cuid FROM user_term_credits_settlements cs WHERE cs.settlement_date = '%s' AND  cs.euid = '%s' AND cs.register_id = '%s'", $cuid->settlement_date, $cuid->euid, $cuid->register_id);

	while($cuid_dp = db_result($cuids)) {
		db_query("UPDATE user_term_credits_usages SET settled = '0' WHERE cuid = '%d'", $cuid_dp);
	}

	pos_api_expose_remove_payment_for_sid($sid);

	db_query("DELETE FROM user_term_credits_settlements WHERE settlement_date = '%s' AND  euid = '%s' AND register_id = '%s'", $cuid->settlement_date, $cuid->euid, $cuid->register_id);
}

function pos_api_expose_remove_payment_for_sid($sid) {
	if($sid) {
		db_query("DELETE FROM user_term_credits_cash_log WHERE sid = '%d'", $sid);

		db_query("DELETE FROM user_term_credits_check_log WHERE sid = '%d'", $sid);
	
		db_query("DELETE FROM user_term_credits_credit_card_log WHERE sid = '%d'", $sid);
	
		db_query("DELETE FROM user_term_credits_money_order_log WHERE sid = '%d'", $sid);

		$rma_pid = db_result(db_query("SELECT rma_pid FROM user_term_credits_rma_log WHERE sid = '%d'", $sid));

		if($rma_pid) {
			db_query("DELETE FROM pos_api_expose_rma_refund_log WHERE pid = '%d'", $rma_pid);
			db_query("DELETE FROM user_term_credits_rma_log WHERE sid = '%d'", $sid);
		}
	}
}
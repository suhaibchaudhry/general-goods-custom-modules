<?php
function user_term_credits_date_range_form($form_state, $timestamp_delta = 15552000) {
	$form = array();
	if($_GET['start_date']) {
		$date = $_GET['start_date']['date'].' '.$_GET['start_date']['time'];
	} else {
		if($timestamp_delta) {
			$timestamp = time()-(int)$timestamp_delta;
		} else {
			$timestamp = time()-15552000;
		}
		$date = date( "Y-m-d H:i", mktime(7, 0, 0, date("n", $timestamp), date("j", $timestamp), date("Y", $timestamp)));
	}

	if($_GET['end_date']) {
		$end_date = $_GET['end_date']['date'].' '.$_GET['end_date']['time'];
	} else {
		$end_date = date( "Y-m-d H:i", mktime(22, 0, 0));
	}

	$format = 'Y-m-d H:i';

	$form['start_date'] = array(
		'#type' => 'date_popup', // types 'date_text' and 'date_timezone' are also supported. See .inc file.
		'#title' => t('Start Date'),
		'#default_value' => $date,
		'#date_format' => $format,
		'#date_label_position' => 'within', // See other available attributes and what they do in date_api_elements.inc
		'#date_timezone' => 'America/Chicago', // Optional, if your date has a timezone other than the site timezone.
		'#date_year_range' => '-3:+3' // Optional, used to set the year range (back 3 years and forward 3 years is the default).
	);

	$form['end_date'] = array(
		'#type' => 'date_popup', // types 'date_text' and 'date_timezone' are also supported. See .inc file.
		'#title' => t('End Date'),
		'#default_value' => $end_date,
		'#date_format' => $format,
		'#date_label_position' => 'within', // See other available attributes and what they do in date_api_elements.inc
		'#date_timezone' => 'America/Chicago', // Optional, if your date has a timezone other than the site timezone.
		'#date_year_range' => '-3:+3' // Optional, used to set the year range (back 3 years and forward 3 years is the default).
	);

	$form['#method'] = 'get';

	$form['apply'] = array(
		'#type' => 'submit',
		'#value' => 'Apply'
	);

	if($_GET['token']) {
		$form['token'] = array(
			'#type' => 'hidden',
			'#value' => $_GET['token']
		);
	}

	if($timestamp_delta) {
		$timestamp_days = floor((int)$timestamp_delta/86400);
	} else {
		$timestamp_days = '180';
	}

	$form['reset_button'] = array(
	    '#type' => 'markup',
	    '#value' => l('Reset to last '.$timestamp_days.' days', $_GET['q'], array('attributes' => array('class' => 'reset-button')))
	);

	return $form;
}

function utc_invoice_history($user, $themeless = false) {
	drupal_add_css(drupal_get_path('module', 'ggw_state_reporting').'/css/sales_report_form.css');

	$profile = content_profile_load('profile', $user->uid);

	if(!$themeless) {
		$header = '<pre>Invoice History: '.$profile->title."\t\tAccount: ".$user->name."</pre>";
		$header .= "<pre>This report lists last 180 days' A/R invoices by default.</pre>";
	}

	$content = $header."\n";
	$content .= drupal_get_form('user_term_credits_date_range_form');

	if($_GET['start_date']) {
		$date = strtotime($_GET['start_date']['date'].' '.$_GET['start_date']['time']);
	} else {
		$timestamp = time()-15552000;
		$date = mktime(7, 0, 0, date("n", $timestamp), date("j", $timestamp), date("Y", $timestamp));
	}

	if($_GET['end_date']) {
		$end_date = strtotime($_GET['end_date']['date'].' '.$_GET['end_date']['time']);
	} else {
		$end_date = mktime(22, 0, 0);
	}

	$invoices = db_query("SELECT tl.pid, tl.timestamp, tl.credit_usage_id, tl.ticket_id, tl.amount, cu.due_date
				 			FROM {pos_api_expose_transaction_log} tl
				 			LEFT JOIN user_term_credits_usages cu ON tl.credit_usage_id = cu.cuid 
	LEFT JOIN uc_orders uo ON uo.order_id = tl.ticket_id
				 			WHERE tl.timestamp >= '%s' AND tl.timestamp <= '%s' AND tl.cuid = '%d' AND uo.order_status = 'pos_completed'", $date, $end_date, $user->uid);

	$header = array('Type', 'Invoice #', 'Ticket #', 'Date', 'Inv. Total', 'Balance', 'Due Date');
	$data = array();

	while($invoice = db_fetch_object($invoices)) {
		$remaining = uc_currency_format(user_term_credits_recursive_round(db_result(db_query("SELECT (cu.credit_amount-IFNULL(pre.paid, 0)) AS balance FROM user_term_credits_usages cu
	                  LEFT JOIN (SELECT cuid, SUM(settlement_amount) AS paid FROM user_term_credits_settlements GROUP BY cuid) pre ON cu.cuid = pre.cuid
					  WHERE cu.cuid = '%d'", $invoice->credit_usage_id))));
		$data[] = array(
			'I',
			$invoice->pid,
			l($invoice->ticket_id, 'admin/store/orders/'.$invoice->ticket_id),
			date('n/j/Y', $invoice->timestamp),
			uc_currency_format(user_term_credits_recursive_round($invoice->amount)),
			$remaining,
			date('n/j/Y', $invoice->due_date)
		);
	}

	$content .= theme('table', $header, $data, array('class' => 'payment-history-table'));

	if($themeless) {
		print theme('payment_history_report', $content, $user->uid, 'Invoice History');
	} else {
		return $content;
	}
}

function utc_payment_history_pos($user) {
	//Avoid Caching Invoice
	header('Cache-Control: no-cache, no-store, must-revalidate'); // HTTP 1.1.
	header('Pragma: no-cache'); // HTTP 1.0.
	header('Expires: 0'); // Proxies.

	$token = $_GET['token'];
	$uid = _pos_api_expose_uid_from_token($token);
	if($uid) {
		utc_payment_history($user, true);
	} else {
		print "Access denied to reciept.";
	}
}

function utc_invoice_history_pos($user) {
	//Avoid Caching Invoice
	header('Cache-Control: no-cache, no-store, must-revalidate'); // HTTP 1.1.
	header('Pragma: no-cache'); // HTTP 1.0.
	header('Expires: 0'); // Proxies.

	$token = $_GET['token'];
	$uid = _pos_api_expose_uid_from_token($token);
	if($uid) {
		utc_invoice_history($user, true);
	} else {
		print "Access denied to reciept.";
	}
}

function utc_customer_statement_pos($user) {
	//Avoid Caching Invoice
	header('Cache-Control: no-cache, no-store, must-revalidate'); // HTTP 1.1.
	header('Pragma: no-cache'); // HTTP 1.0.
	header('Expires: 0'); // Proxies.

	$token = $_GET['token'];
	$uid = _pos_api_expose_uid_from_token($token);
	if($uid) {
		utc_customer_statement($user, true);
	} else {
		print "Access denied to reciept.";
	}
}

function utc_payment_history($user, $themeless = false) {
	drupal_add_css(drupal_get_path('module', 'ggw_state_reporting').'/css/sales_report_form.css');

	$profile = content_profile_load('profile', $user->uid);

	if(!$themeless) {
		$header = '<pre>Payment History: '.$profile->title."\t\tAccount: ".$user->name."</pre>";
		$header .= "<pre>This report lists last 180 days' A/R payments by default. (not over-the-counter payments)</pre>";
	}
	$content = $header."\n";
	$content .= drupal_get_form('user_term_credits_date_range_form');

	if($_GET['start_date']) {
		$date = strtotime($_GET['start_date']['date'].' '.$_GET['start_date']['time']);
	} else {
		$timestamp = time()-15552000;
		$date = mktime(7, 0, 0, date("n", $timestamp), date("j", $timestamp), date("Y", $timestamp));
	}

	if($_GET['end_date']) {
		$end_date = strtotime($_GET['end_date']['date'].' '.$_GET['end_date']['time']);
	} else {
		$end_date = mktime(22, 0, 0);
	}

	$settlements_sql = "SELECT s.sid, u.uid, SUM(s.settlement_amount) AS settlement_amount, GROUP_CONCAT(u.order_id SEPARATOR ',') AS invoice_ids, GROUP_CONCAT(CAST(s.settlement_date as SIGNED)-CAST(u.consumption_date as SIGNED) SEPARATOR ',') AS days, GROUP_CONCAT(s.cuid SEPARATOR ',') AS cuids, s.payment_type, s.remarks, s.settlement_date FROM (SELECT * FROM user_term_credits_settlements si ORDER BY si.sid ASC) s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' AND u.uid = '%d' GROUP BY u.uid, s.settlement_date ORDER BY s.settlement_date DESC";

	$settlements = db_query($settlements_sql, $date, $end_date, $user->uid);
	$header = array('Payment Date', 'Payment', 'Invoice #', 'Invoice Amnt.', 'Days', 'Remaining Bal.', 'Payby', 'Amount');
	$payment_types = array(
		'pos_payment' => 'Register Payment',
		'cash' => 'Cash - B.O.',
		'check' => 'Check - B.O.',
		'money_order' => 'M.O. - B.O.',
		'post_check' => 'Post Dated Check - B.O.'
	);

	$rows = array();
	while($settlement = db_fetch_object($settlements)) {
		$transacs_count++;

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
						$stashed_rma_change = bcadd(trim($stash->change), trim($stashed_rma_change), 6);
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

		$invoice_links = explode(',', $settlement->invoice_ids);
		$invoices = '';
		foreach($invoice_links as $link) {
			$invoices .= l($link, 'admin/store/orders/'.$link).'<br />';
		}

		$settlement_days = explode(',', $settlement->days);
		$days = '';
		foreach($settlement_days as $day) {
			$days .= floor(($day)/86430).'<br />';
		}

		$cuids = explode(',', $settlement->cuids);
		$remaining = '';
		$invoiced = '';
		foreach($cuids as $cuid) {
			$remaining .= uc_currency_format(user_term_credits_recursive_round(db_result(db_query("SELECT (cu.credit_amount-IFNULL(pre.paid, 0)) AS balance FROM user_term_credits_usages cu
	                  LEFT JOIN (SELECT cuid, SUM(settlement_amount) AS paid FROM user_term_credits_settlements GROUP BY cuid) pre ON cu.cuid = pre.cuid
					  WHERE cu.cuid = '%d'", $cuid)))).'<br />';
			$invoiced .= uc_currency_format(user_term_credits_recursive_round(db_result(db_query("SELECT credit_amount FROM user_term_credits_usages WHERE cuid = '%s'", $cuid)))).'<br />';
		}

		$rows[] = array(
			date('n/j/Y', $settlement->settlement_date),
			uc_currency_format(user_term_credits_recursive_round($settlement->settlement_amount), false),
			$invoices,
			$invoiced,
			$days,
			$remaining,
			$column_1,
			$column_2
		);
	}

	$content .= theme('table', $header, $rows, array('class' => 'payment-history-table'));

	if($themeless) {
		print theme('payment_history_report', $content, $user->uid, 'Payment History');
	} else {
		return $content;
	}
}

function utc_customer_statement($user, $themeless = false) {
	drupal_add_css(drupal_get_path('module', 'ggw_state_reporting').'/css/sales_report_form.css');

	$profile = content_profile_load('profile', $user->uid);

	$content = $header."\n";
	$content .= drupal_get_form('user_term_credits_date_range_form');

	if($_GET['start_date']) {
		$date = strtotime($_GET['start_date']['date'].' '.$_GET['start_date']['time']);
	} else {
		$timestamp = time()-15552000;
		$date = mktime(7, 0, 0, date("n", $timestamp), date("j", $timestamp), date("Y", $timestamp));
	}

	if($_GET['end_date']) {
		$end_date = strtotime($_GET['end_date']['date'].' '.$_GET['end_date']['time']);
	} else {
		$end_date = mktime(22, 0, 0);
	}

	$start_credits = _user_term_credits_getCredits($user->uid, $date);
	$end_credits = _user_term_credits_getCredits($user->uid, $end_date);
	$current_credits = _user_term_credits_getCredits($user->uid); 

	$content .= '<p style="clear: both;"></p><pre>Customer Statement: '.$profile->title."\t\tAccount: ".$user->name."</pre>";
	$content .= '<pre>'.str_pad('Beginning Balance', 70, '. ').$start_credits->pending_payments_view.'</pre>';

	$transaction_bucket = array();
	$invoices = db_query("SELECT tl.pid, tl.timestamp, tl.credit_usage_id, tl.ticket_id, cu.credit_amount AS amount, tl.amount as full_amount, cu.due_date, cu.settled
				 			FROM {pos_api_expose_transaction_log} tl
				 			LEFT JOIN user_term_credits_usages cu ON tl.credit_usage_id = cu.cuid 
				 			WHERE tl.timestamp >= '%s' AND tl.timestamp <= '%s' AND tl.cuid = '%d'", $date, $end_date, $user->uid);
	while($invoice = db_fetch_object($invoices)) {
		$transaction_bucket[] = array(
			'type' => 'invoice',
			'timestamp' => $invoice->timestamp,
			'full_amount' => $invoice->full_amount,
			'amount' => $invoice->amount,
			'invoice_id' => $invoice->pid,
			'ticket_id' => $invoice->ticket_id,
			'cuid' => $invoice->credit_usage_id,
			'settled' => $invoice->settled,
			'due_date' => $invoice->due_date
		);
	}

	$settlements_sql = "SELECT s.sid, u.uid, SUM(s.settlement_amount) AS settlement_amount, s.payment_type, s.remarks, s.settlement_date FROM (SELECT * FROM user_term_credits_settlements si ORDER BY si.sid ASC) s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' AND u.uid = '%d' GROUP BY u.uid, s.settlement_date ORDER BY s.settlement_date DESC";

	$settlements = db_query($settlements_sql, $date, $end_date, $user->uid);

	$payment_types = array(
		'pos_payment' => 'Register Payment',
		'cash' => 'Cash - B.O.',
		'check' => 'Check - B.O.',
		'money_order' => 'M.O. - B.O.',
		'post_check' => 'Post Dated Check - B.O.'
	);

	$rows = array();
	while($settlement = db_fetch_object($settlements)) {
		$column_1 = '';
		$column_2 = '';
		$method = array();
		$payment = array();

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
						$stashed_rma_change = bcadd(trim($stash->change), trim($stashed_rma_change), 6);
						$column_1 .= 'RMA SENT ';
					}
				}

				//if($components[0] != 'CASH' && $components[0] != 'CHANGE' && $components[1] > 0) {
					$column_1 .= $components[0];
					$column_2 .= $components[1];

					$method[] = $column_1;
					$payment[] = $column_2;
				//}

				$column_1 = '';
				$column_2 = '';
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
				$column_2 .= $components[1];

				$method[] = $column_1;
				$payment[] = $column_2;

				$column_1 = '';
				$column_2 = '';
			}
			$i++;
		}

		if(empty($method) && empty($payment)) {
			$transaction_bucket[] = array(
				'type' => 'settlement',
				'timestamp' => $settlement->settlement_date,
				'method' => array(strtoupper($settlement->payment_type)),
				'payment' => array($settlement->settlement_amount)
			);
		} else {
			$transaction_bucket[] = array(
				'type' => 'settlement',
				'timestamp' => $settlement->settlement_date,
				'method' => $method,
				'payment' => $payment
			);
		}
	}

	usort($transaction_bucket, 'utc_compare_statement_items');

	$header = array('Date', 'Invoice # / Desc.', 'Charges.', 'Payment', 'Balance.');
	$rows = array();

	$age_header = array('Invoice', 'Date', 'Amount', 'Due', 'Current', 'In-30', 'In-60', 'In-90', 'In-120', '120+');
	$age_rows = array();

	$balance = $start_credits->pending_payments;
	foreach($transaction_bucket as $transaction) {
		if($transaction['type'] == 'invoice') {
			$balance = bcadd(trim($balance), trim($transaction['amount']), 6);

			if($transaction['settled'] == '0') {
				$row = array(
					$transaction['invoice_id'].'/'.l($transaction['ticket_id'], 'admin/store/orders/'.$transaction['ticket_id']),
					date('n/j/Y', $transaction['timestamp']),
					uc_currency_format(user_term_credits_recursive_round($transaction['full_amount'])),
					date('n/j/Y', $transaction['due_date']),
					'',
					'',
					'',
					'',
					'',
					''
				);

				$date_diff = (time()-$transaction['due_date']);
				if($date_diff < 0) {
					$index = 0;
				} else {
					$days = floor($date_diff/(60*60*24));
					if($days >= 0 && $days <= 30) {
						$index = 1;
					} elseif ($days > 30 && $days <= 60) {
						$index = 2;
					} elseif ($days > 60 && $days <= 90) {
						$index = 3;
					} elseif ($days > 90 && $days <= 120) {
						$index = 4;
					} elseif ($days > 120) {
						$index = 5;
					}
				}

				$remaining = uc_currency_format(user_term_credits_recursive_round(db_result(db_query("SELECT (cu.credit_amount-IFNULL(pre.paid, 0)) AS balance FROM user_term_credits_usages cu
	                  LEFT JOIN (SELECT cuid, SUM(settlement_amount) AS paid FROM user_term_credits_settlements GROUP BY cuid) pre ON cu.cuid = pre.cuid
					  WHERE cu.cuid = '%d'", $transaction['cuid']))));

				$row[$index+4] = $remaining;
				$age_rows[] = $row;
			}

			$payment = '';

			$transacs = db_query("SELECT tl.credit_usage_id, tl.cash_pid, tl.rma_pid, tl.cc_pid, tl.check_pid, tl.mo_pid, tl.amount, tl.pid, tl.cuid, tl.uid, tl.timestamp, tl.register_id, tl.ticket_id, cl.amount_paid AS cash_paid, cl.change AS change_made, cl.total AS cash_total, chl.paid_amount AS check_payment, chl.check_number, chl.post_dated AS check_post_dated, chl.cash_date AS check_cash_date, ccl.amount_paid AS cc_amount, mo.paid_amount AS mo_paid_amount, mo.reference AS mo_ref, cu.credit_amount AS ar_payment_amount, ccl.transaction_id FROM {pos_api_expose_transaction_log} tl LEFT JOIN {pos_api_expose_cash_log} cl ON tl.cash_pid = cl.pid LEFT JOIN {pos_api_expose_credit_card_log} ccl ON tl.cc_pid = ccl.pid LEFT JOIN {pos_api_expose_check_log} chl ON tl.check_pid = chl.pid LEFT JOIN {pos_api_expose_money_order_log} mo ON tl.mo_pid = mo.pid LEFT JOIN {user_term_credits_usages} cu ON tl.credit_usage_id = cu.cuid WHERE tl.pid = '%d'", $transaction['invoice_id']);

			$transac = db_fetch_object($transacs);

			$payment = '';
			if($transac->cash_pid) {
				if($transac->cash_paid > 0 || $transac->change_made > 0) {
					$payment .= '<strong>CASH:</strong> ';
					$payment .= '<strong>CHANGE:</strong> ';
					$payment .= uc_currency_format(user_term_credits_recursive_round($transac->cash_paid), false)."\n";
					$payment .= uc_currency_format(user_term_credits_recursive_round($transac->change_made), false)."\n";
				}
			}

			if($transac->cc_pid) {
				$payment .= '<strong>Credit Card';
				if($transac->transaction_id) {
					$payment .= ' - '.$transac->transaction_id;
				}
				$payment .= ':</strong> ';
				$payment .= uc_currency_format(user_term_credits_recursive_round($transac->cc_amount), false)."\n";
			}

			if($transac->check_pid) {
				if($transac->check_post_dated) {
					$payment .= '<strong>POST DATED CHECK';
					if($transac->check_number) {
						$payment .= ' - '.$transac->check_number;
					}
					$payment .= ':</strong> ';
					$payment .= uc_currency_format(user_term_credits_recursive_round($transac->check_payment), false)."\n";
					$payment .= ' - '.$transac->check_cash_date."\n";
				} else {
					$payment .= '<strong>CHECK';
					if($transac->check_number) {
						$payment .= ' - '.$transac->check_number;
					}
					$payment .= ':</strong> ';

					$payment .= uc_currency_format(user_term_credits_recursive_round($transac->check_payment), false)."\n";
				}
			}

			if($transac->mo_pid) {
				if($transac->mo_ref) {
					$payment .= '<strong>Money Order - '.$transac->mo_ref.':</strong> ';
				} else {
					$payment .= '<strong>Money Order:</strong> ';
				}
				$payment .= uc_currency_format(user_term_credits_recursive_round($transac->mo_paid_amount), false)."\n";
			}

			if($transac->credit_usage_id) {
				$payment .= '<strong>AR:</strong> ';
				$payment .= uc_currency_format(user_term_credits_recursive_round($transac->ar_payment_amount), false)."\n";
			}

			if($transac->rma_pid < 0) {
				$payment .= '<strong>RMA CASH OUT:</strong> ';

				$transac->amount = (-1)*$transac->amount;
				$payment .= uc_currency_format(user_term_credits_recursive_round($transac->amount), false)."\n";
			} else if($transac->rma_pid > 0) {
				$rma = db_result(db_query("SELECT (-1)*total_refund FROM pos_api_expose_rma_refund_log WHERE pid = '%d'", $transac->rma_pid));
				if($rma > 0) {
					$payment .= '<strong>RMA DEBIT:</strong> ';
				} else {
					$payment .= '<strong>RMA CREDIT:</strong> ';
				}
				$payment .= uc_currency_format(user_term_credits_recursive_round($rma), false)."\n";
			}

			$rows[] = array(
				date('n/j/Y', $transaction['timestamp']),
				$transaction['invoice_id'].'/'.l($transaction['ticket_id'], 'admin/store/orders/'.$transaction['ticket_id']),
				uc_currency_format(user_term_credits_recursive_round($transaction['full_amount'])),
				$payment,
				uc_currency_format(user_term_credits_recursive_round($balance))
			);
		} else {
			$column_1 = '';
			$column_2 = '';
			$bal_col = '';
			foreach($transaction['method'] as $method) {
				$column_1 .= $method.'<br />';
			}
			$i = 0;
			foreach($transaction['payment'] as $method) {
				$column_2 .= uc_currency_format(user_term_credits_recursive_round($method)).'<br />';
				if($transaction['method'][$i] == 'CHANGE' || $transaction['method'][$i] == 'RMA SENT CHANGE') {
					$balance = bcadd(trim($balance), trim($method), 6);
					$bal_col .= uc_currency_format(user_term_credits_recursive_round($balance)).'<br />';
				} else {
					$balance = bcsub(trim($balance), trim($method), 6);
					if($transaction['method'][$i] == 'CASH' && (in_array('CHANGE', $transaction['method']) || in_array('RMA SENT CHANGE', $transaction['method']))) {
						$bal_col .= '<br />';
					} else {
						$bal_col .= uc_currency_format(user_term_credits_recursive_round($balance)).'<br />';
					}
				}
				$i++;
			}
			$rows[] = array(
				date('n/j/Y', $transaction['timestamp']),
				'<strong>'.$column_1.'</strong>',
				'',
				$column_2,
				$bal_col
			);
		}
	}

	$content .= theme('table', $header, $rows, array('class' => 'payment-history-table'));

	$content .= '<pre>'.str_pad('Ending Balance', 70, ' .').$end_credits->pending_payments_view.'</pre>';

	$content .= '<br /><pre>Aging Status:</pre>';

	$content .= theme('table', $age_header, $age_rows, array('class' => 'payment-history-table'));

	$content .= '<pre>Aging Summary: '.$current_credits->pending_payments_view.'</pre>';

	if($themeless) {
		print theme('payment_history_report', $content, $user->uid, 'Customer Statement');
	} else {
		return $content;
	}
}

function utc_compare_statement_items($a, $b) {
	return ((int)$a['timestamp'])-((int)$b['timestamp']);
}

function utc_customer_activity_report() {
	drupal_add_css(drupal_get_path('module', 'ggw_state_reporting').'/css/sales_report_form.css');

	$profile = content_profile_load('profile', $user->uid);

	$content = $header."\n";
	$content .= drupal_get_form('user_term_credits_date_range_form', 2592000);

	if($_GET['start_date']) {
		$date = strtotime($_GET['start_date']['date'].' '.$_GET['start_date']['time']);
	} else {
		$timestamp = time()-2592000;
		$date = mktime(7, 0, 0, date("n", $timestamp), date("j", $timestamp), date("Y", $timestamp));
	}

	if($_GET['end_date']) {
		$end_date = strtotime($_GET['end_date']['date'].' '.$_GET['end_date']['time']);
	} else {
		$end_date = mktime(22, 0, 0);
	}

	$invoices = "SELECT tl.cuid AS customer_uid, tl.timestamp AS etime
				 			FROM {pos_api_expose_transaction_log} tl 
				 			WHERE tl.credit_usage_id <> 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' GROUP BY tl.cuid";

	$settlements_sql = "SELECT u.uid AS customer_uid, s.settlement_date AS etime FROM user_term_credits_settlements s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' GROUP BY u.uid, s.settlement_date";

	$sql = "SELECT p.customer_uid, MAX(p.etime) AS latest_interaction FROM (".$invoices." UNION ".$settlements_sql.") p GROUP BY customer_uid ORDER BY latest_interaction DESC";

	$customers = db_query($sql, $date, $end_date, $date, $end_date);

	$header = array('Customer', 'Phone #', 'Previous Balance', 'Receipt', 'Payment', 'Current Balance', 'Tax Amount');
	$rows = array();
	$total_previous = '0';
	$total_receipt = '0';
	$total_payment = '0';
	$current_outstanding = '0';
	$total_tax = '0';

	while($customer = db_fetch_object($customers)) {
		$profile = content_profile_load('profile', $customer->customer_uid);
		$start_account = _user_term_credits_getCredits($customer->customer_uid, $date);
		$end_account = _user_term_credits_getCredits($customer->customer_uid, $end_date);

		$total_previous = bcadd(trim($total_previous), trim($start_account->pending_payments), 6);
		$current_outstanding = bcadd(trim($current_outstanding), trim($end_account->pending_payments), 6);

		$receipt_amount = db_result(db_query("SELECT SUM(tl.amount)
				 			FROM {pos_api_expose_transaction_log} tl 
				 			WHERE tl.credit_usage_id <> 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' AND tl.cuid = '%d'", $date, $end_date, $customer->customer_uid));

		if($receipt_amount) {
			$total_receipt = bcadd(trim($total_receipt), trim($receipt_amount), 6);
		}

		$payment_amount = db_result(db_query("SELECT SUM(s.settlement_amount) FROM user_term_credits_settlements s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' AND u.uid = '%d' GROUP BY u.uid", $date, $end_date, $customer->customer_uid));

		if($payment_amount) {
			$total_payment = bcadd(trim($total_payment), trim($payment_amount), 6);
		}

		$tax_amount = db_result(db_query("SELECT SUM(ucl.amount)
				 			FROM {pos_api_expose_transaction_log} tl
				 			INNER JOIN {uc_order_line_items} ucl ON tl.ticket_id = ucl.order_id 
				 			WHERE tl.credit_usage_id <> 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' AND tl.cuid = '%d'", $date, $end_date, $customer->customer_uid));

		if($tax_amount) {
			$total_tax = bcadd(trim($total_tax), trim($tax_amount), 6);
		}

		if($receipt_amount) {
			$receipt_amount = uc_currency_format(user_term_credits_recursive_round($receipt_amount), false);
		} else {
			$receipt_amount = '';
		}

		if($payment_amount) {
			$payment_amount = uc_currency_format(user_term_credits_recursive_round($payment_amount), false);
		} else {
			$payment_amount = '';
		}

		if($tax_amount) {
			$tax_amount = uc_currency_format(user_term_credits_recursive_round($tax_amount), false);
		} else {
			$tax_amount = '';
		}

		$rows[] = array(
			$profile->name.'- '.substr($profile->title, 0, 24),
			format_phone_number($profile->field_profile_phone[0]['number']),
			substr($start_account->pending_payments_view, 1),
			$receipt_amount,
			$payment_amount,
			substr($end_account->pending_payments_view, 1),
			$tax_amount
		);
	}

	$rows[] = array(
		'',
		'',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($total_previous), false).'</strong>',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($total_receipt), false).'</strong>',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($total_payment), false).'</strong>',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($current_outstanding), false).'</strong>',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($total_tax), false).'</strong>'
	);

	$content .= theme('table', $header, $rows);

	return $content;
}

function format_phone_number($data) {
	if(  preg_match( '/^(\d{3})(\d{3})(\d{4})$/', $data,  $matches ) ) {
	    $result = '('.$matches[1] . ') ' .$matches[2] . '-' . $matches[3];
	    return $result;
	}
}

function full_utc_customer_activity_report() {
	drupal_add_css(drupal_get_path('module', 'ggw_state_reporting').'/css/sales_report_form.css');

	$profile = content_profile_load('profile', $user->uid);

	$content = $header."\n";
	$content .= drupal_get_form('user_term_credits_date_range_form', 2592000);

	if($_GET['start_date']) {
		$date = strtotime($_GET['start_date']['date'].' '.$_GET['start_date']['time']);
	} else {
		$timestamp = time()-2592000;
		$date = mktime(7, 0, 0, date("n", $timestamp), date("j", $timestamp), date("Y", $timestamp));
	}

	if($_GET['end_date']) {
		$end_date = strtotime($_GET['end_date']['date'].' '.$_GET['end_date']['time']);
	} else {
		$end_date = mktime(22, 0, 0);
	}

	$sql = "SELECT tl.cuid AS customer_uid, MAX(tl.timestamp) AS etime
				 			FROM {pos_api_expose_transaction_log} tl 
				 			WHERE tl.credit_usage_id <> 0 AND tl.timestamp <= '%s' GROUP BY tl.cuid";

	/*$settlements_sql = "SELECT u.uid AS customer_uid, s.settlement_date AS etime FROM user_term_credits_settlements s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' GROUP BY u.uid, s.settlement_date";

	$sql = "SELECT p.customer_uid, MAX(p.etime) AS latest_interaction FROM (".$invoices." UNION ".$settlements_sql.") p GROUP BY customer_uid ORDER BY latest_interaction DESC";*/

	$customers = db_query($sql, $end_date);

	$header = array('Customer', 'Phone #', 'Previous Balance', 'Receipt', 'Payment', 'Current Balance', 'Tax Amount');
	$rows = array();
	$total_previous = '0';
	$total_receipt = '0';
	$total_payment = '0';
	$current_outstanding = '0';
	$total_tax = '0';

	while($customer = db_fetch_object($customers)) {
		$display_flag = false;
		$profile = content_profile_load('profile', $customer->customer_uid);
		$start_account = _user_term_credits_getCredits($customer->customer_uid, $date);
		$end_account = _user_term_credits_getCredits($customer->customer_uid, $end_date);

		$total_previous = bcadd(trim($total_previous), trim($start_account->pending_payments), 6);
		$current_outstanding = bcadd(trim($current_outstanding), trim($end_account->pending_payments), 6);

		if($start_account->pending_payments > 0 || $end_account->pending_payments > 0) {
			$display_flag = true;
		}

		$receipt_amount = db_result(db_query("SELECT SUM(tl.amount)
				 			FROM {pos_api_expose_transaction_log} tl 
				 			WHERE tl.credit_usage_id <> 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' AND tl.cuid = '%d'", $date, $end_date, $customer->customer_uid));

		if($receipt_amount) {
			$display_flag = true;
			$total_receipt = bcadd(trim($total_receipt), trim($receipt_amount), 6);
		}

		$payment_amount = db_result(db_query("SELECT SUM(s.settlement_amount) FROM user_term_credits_settlements s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' AND u.uid = '%d' GROUP BY u.uid", $date, $end_date, $customer->customer_uid));

		if($payment_amount) {
			$display_flag = true;
			$total_payment = bcadd(trim($total_payment), trim($payment_amount), 6);
		}

		$tax_amount = db_result(db_query("SELECT SUM(ucl.amount)
				 			FROM {pos_api_expose_transaction_log} tl
				 			INNER JOIN {uc_order_line_items} ucl ON tl.ticket_id = ucl.order_id 
				 			WHERE tl.credit_usage_id <> 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' AND tl.cuid = '%d'", $date, $end_date, $customer->customer_uid));

		if($tax_amount) {
			$display_flag = true;
			$total_tax = bcadd(trim($total_tax), trim($tax_amount), 6);
		}

		if($receipt_amount) {
			$receipt_amount = uc_currency_format(user_term_credits_recursive_round($receipt_amount), false);
		} else {
			$receipt_amount = '';
		}

		if($payment_amount) {
			$payment_amount = uc_currency_format(user_term_credits_recursive_round($payment_amount), false);
		} else {
			$payment_amount = '';
		}

		if($tax_amount) {
			$tax_amount = uc_currency_format(user_term_credits_recursive_round($tax_amount), false);
		} else {
			$tax_amount = '';
		}

		if($display_flag) {
			$rows[] = array(
				$profile->name.'- '.substr($profile->title, 0, 24),
				format_phone_number($profile->field_profile_phone[0]['number']),
				substr($start_account->pending_payments_view, 1),
				$receipt_amount,
				$payment_amount,
				substr($end_account->pending_payments_view, 1),
				$tax_amount
			);
		}
	}

	$rows[] = array(
		'',
		'',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($total_previous), false).'</strong>',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($total_receipt), false).'</strong>',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($total_payment), false).'</strong>',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($current_outstanding), false).'</strong>',
		'<strong>'.uc_currency_format(user_term_credits_recursive_round($total_tax), false).'</strong>'
	);

	$content .= theme('table', $header, $rows);

	return $content;
}

function _pos_api_expose_category_report() {
	drupal_add_css(drupal_get_path('module', 'ggw_state_reporting').'/css/sales_report_form.css');
	drupal_add_css(drupal_get_path('module', 'ggw_qr_codes').'/css/ggw_qr_codes.css');

	$content .= drupal_get_form('user_term_credits_date_range_form', 2592000);

	if($_GET['start_date']) {
		$date = strtotime($_GET['start_date']['date'].' '.$_GET['start_date']['time']);
	} else {
		$timestamp = time()-2592000;
		$date = mktime(7, 0, 0, date("n", $timestamp), date("j", $timestamp), date("Y", $timestamp));
	}

	if($_GET['end_date']) {
		$end_date = strtotime($_GET['end_date']['date'].' '.$_GET['end_date']['time']);
	} else {
		$end_date = mktime(22, 0, 0);
	}

	$tree = taxonomy_get_tree(7, 0, -1, 1);
	$content .= '<div class="qr-by-dept" style="white-space: pre;">';
	$content .= '<strong>'.ggw_state_reporting_msa_cell('Category', 100);
    $content .= ggw_state_reporting_msa_cell('Total Sales Amount', 40);
    $content .= ggw_state_reporting_msa_cell('Total G/P', 40);
    $content .= ggw_state_reporting_msa_cell('Product Cost', 40);
    $content .= ggw_state_reporting_msa_cell('Margin', 40).'</strong>';
	$content .= _pos_api_expose_category_report_tree($tree, $start_date, $end_date).'</div>';

	return $content;
}

function _pos_api_expose_category_report_tree($tree, $start_date, $end_date) {
    if (count($tree)) {
        $output = '<ul class="taxonomy-tree" style="margin: 0;">';
        foreach ($tree as $term) {
            $output .= '<li class="taxonomy-term">';

            $stats = _pos_api_expose_get_cat_stats($term->tid, $start_date, $end_date);

            if($stats->sales) {
            	$price = number_format($stats->sales, 2);
			} else {
				$price = '';
			}

	        $gross = $stats->sales-$stats->costs ? number_format($stats->sales-$stats->costs, 2) : '';

            if($stats->costs) {
	            $cost = number_format($stats->costs, 2);
        	} else {
        		$cost = '';
        	}

            if($stats->sales) {
            	$markup = number_format(100+(100-($stats->costs*100))/$stats->sales, 2).'%';
        	} else {
        		$markup = '';
        	}


            $output .= ggw_state_reporting_msa_cell($term->name, 100);
            $output .= ggw_state_reporting_msa_cell($price, 40);
            $output .= ggw_state_reporting_msa_cell($gross, 40);
            $output .= ggw_state_reporting_msa_cell($cost, 40);
            $output .= ggw_state_reporting_msa_cell($markup, 40);

            $term->children = taxonomy_get_children($term->tid);
                        if ($term->children) {
                                $output .= _pos_api_expose_category_report_tree($term->children, $start_date, $end_date);
            }
            $output .= '</li>';
        }
        $output .= '</ul>';
    }
    return $output;
}

function _pos_api_expose_get_cat_stats($tid, $start_date, $end_date) {
	return db_fetch_object(db_query("SELECT SUM(qty*price) AS sales, SUM(qty*cost) AS costs FROM uc_order_products uop
		INNER JOIN uc_orders uo ON uo.order_id = uop.order_id
		INNER JOIN term_node tn ON tn.nid = uop.nid
		WHERE uo.order_status = 'pos_completed' AND tn.tid = '%d' AND uo.modified >= '%s' AND uo.modified <= '%s'", $tid, $start_date, $end_date));
}
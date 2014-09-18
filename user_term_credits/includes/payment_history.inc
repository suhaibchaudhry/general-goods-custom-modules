<?php
function user_term_credits_date_range_form() {
	$form = array();
	if($_GET['start_date']) {
		$date = $_GET['start_date']['date'].' '.$_GET['start_date']['time'];
	} else {
		$timestamp = time()-15552000;
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

	$form['reset_button'] = array(
	    '#type' => 'markup',
	    '#value' => l('Reset to last 180 days', $_GET['q'], array('attributes' => array('class' => 'reset-button')))
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
				 			INNER JOIN user_term_credits_usages cu ON tl.credit_usage_id = cu.cuid 
				 			WHERE tl.credit_usage_id <> 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' AND tl.cuid = '%d'", $date, $end_date, $user->uid);

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

	$settlements_sql = "SELECT s.sid, u.uid, SUM(s.settlement_amount) AS settlement_amount, GROUP_CONCAT(u.order_id SEPARATOR ',') AS invoice_ids, GROUP_CONCAT(s.settlement_date-u.consumption_date SEPARATOR ',') AS days, GROUP_CONCAT(s.cuid SEPARATOR ',') AS cuids, s.payment_type, s.remarks, s.settlement_date FROM user_term_credits_settlements s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' AND u.uid = '%d' GROUP BY u.uid, s.settlement_date ORDER BY s.settlement_date DESC";

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
	$invoices = db_query("SELECT tl.pid, tl.timestamp, tl.credit_usage_id, tl.ticket_id, tl.amount, cu.due_date, cu.settled
				 			FROM {pos_api_expose_transaction_log} tl
				 			INNER JOIN user_term_credits_usages cu ON tl.credit_usage_id = cu.cuid 
				 			WHERE tl.credit_usage_id <> 0 AND tl.timestamp >= '%s' AND tl.timestamp <= '%s' AND tl.cuid = '%d'", $date, $end_date, $user->uid);
	while($invoice = db_fetch_object($invoices)) {
		$transaction_bucket[] = array(
			'type' => 'invoice',
			'timestamp' => $invoice->timestamp,
			'amount' => $invoice->amount,
			'invoice_id' => $invoice->pid,
			'ticket_id' => $invoice->ticket_id,
			'cuid' => $invoice->credit_usage_id,
			'settled' => $invoice->settled,
			'due_date' => $invoice->due_date
		);
	}

	$settlements_sql = "SELECT s.sid, u.uid, SUM(s.settlement_amount) AS settlement_amount, s.payment_type, s.remarks, s.settlement_date FROM user_term_credits_settlements s LEFT JOIN user_term_credits_usages u ON s.cuid = u.cuid WHERE s.settlement_date >= '%s' AND s.settlement_date <= '%s' AND u.uid = '%d' GROUP BY u.uid, s.settlement_date ORDER BY s.settlement_date DESC";

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
						$stashed_rma_change = bcadd($stash->change, $stashed_rma_change, 6);
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
			$balance = bcadd($balance, $transaction['amount'], 6);

			if($transaction['settled'] == '0') {
				$row = array(
					$transaction['invoice_id'].'/'.l($transaction['ticket_id'], 'admin/store/orders/'.$transaction['ticket_id']),
					date('n/j/Y', $transaction['timestamp']),
					uc_currency_format(user_term_credits_recursive_round($transaction['amount'])),
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

			$rows[] = array(
				date('n/j/Y', $transaction['timestamp']),
				$transaction['invoice_id'].'/'.l($transaction['ticket_id'], 'admin/store/orders/'.$transaction['ticket_id']),
				uc_currency_format(user_term_credits_recursive_round($transaction['amount'])),
				'',
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
				if($transaction['method'][$i] == 'CHANGE') {
					$balance = bcadd($balance, $method, 6);
					$bal_col .= uc_currency_format(user_term_credits_recursive_round($balance)).'<br />';
				} else {
					$balance = bcsub($balance, $method, 6);
					$bal_col .= uc_currency_format(user_term_credits_recursive_round($balance)).'<br />';
				}
				$i++;
			}
			$rows[] = array(
				date('n/j/Y', $transaction['timestamp']),
				$column_1,
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


	return $content;
}
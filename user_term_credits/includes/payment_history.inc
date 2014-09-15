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
			$invoice->ticket_id,
			date('n/j/Y', $invoice->timestamp),
			uc_currency_format(user_term_credits_recursive_round($invoice->amount)),
			$remaining,
			date('n/j/Y', $invoice->due_date)
		);
	}

	$content .= theme('table', $header, $data, array('class' => 'payment-history-table'));

	return $content;
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
		print theme('payment_history_report', $content, $user->uid);
	} else {
		return $content;
	}
}
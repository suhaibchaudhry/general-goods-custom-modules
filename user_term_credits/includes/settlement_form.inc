<?php
function utc_perform_settlement($user, $cuid) {
	return drupal_get_form('utc_perform_settlement_form', $user, $cuid);
}

function utc_perform_settlement_form(&$form_state, $user, $cuid) {
	drupal_add_css(drupal_get_path('module', 'user_term_credits').'/css/settlement_form.css');
	$form = array();
	$header = array('Settlement Date', 'Amount', 'Payment Type', 'Pay by', 'Payment');
	$data = array();

	$settlements = db_query("SELECT settlement_amount, payment_type, remarks, settlement_date FROM {user_term_credits_settlements} WHERE cuid = '%d' ORDER BY settlement_date DESC", $cuid);

	$payment_types_map = _utc_get_payment_map();
	while($settlement = db_fetch_object($settlements)) {
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


		$data[] = array(
			 date('n/j/Y h:iA', $settlement->settlement_date),
			 '$'.number_format($settlement->settlement_amount, 2),
			 $payment_types_map[$settlement->payment_type],
			 $column_1,
			 $column_2
		);
	}

	$settlements_table = theme('table', $header, $data);

	$credit_usage = db_fetch_object(db_query("SELECT order_id, credit_amount, consumption_date, due_date FROM user_term_credits_usages WHERE cuid = '%d'", $cuid));

	$partial_payments = db_result(db_query("SELECT SUM(settlement_amount) FROM {user_term_credits_settlements} WHERE cuid = '%d'", $cuid));

	$amount_left = $credit_usage->credit_amount-$partial_payments;
	
	$content = '<div class="info"><span class="label">Ticket #</span>'.l($credit_usage->order_id, 'admin/store/orders/'.$credit_usage->order_id).'</div>';

	$content .= '<div class="info"><span class="label">Customer: </span>'.l($user->name, 'user/'.$user->uid.'/term-credits').'</div>';

	$content .= '<div class="info"><span class="label">Invoice Total: </span>'.variable_get('uc_currency_sign', '$').number_format($credit_usage->credit_amount, 2).'</div>';

	$content .= '<div class="info"><span class="label">Amount Settled: </span>'.variable_get('uc_currency_sign', '$').number_format($partial_payments, 2).'</div>';

	$content .= '<div class="info"><span class="label">Amount Left: </span>'.variable_get('uc_currency_sign', '$').number_format($amount_left, 2).'</div>';

	$form['settlements_table'] = array(
		'#type' => 'markup',
		'#value' => $settlements_table
	);

	$form['settlement'] = array(
		'#type' => 'fieldset',
		'#title' => 'Create Settlement',
		'#attributes' => array('class' => 'settlement'),
		'#tree' => true
	);

	$form['cuid'] = array(
		'#type' => 'value',
		'#value' => $cuid
	);

	$form['uid'] = array(
		'#type' => 'value',
		'#value' => $user->uid
	);

	$form['amount_left'] = array(
		'#type' => 'value',
		'#value' => $amount_left
	);

	$form['settlement']['info'] = array(
		'#type' => 'markup',
		'#value' => $content
	);

	$form['settlement']['amount'] = array(
		'#type' => 'textfield',
		'#title' => 'Amount Settled',
		'#field_prefix' => '$',
		'#default_value' => number_format($amount_left, 2, '.', '')
	);

	$form['settlement']['type'] = array(
		'#type' => 'select',
		'#title' => 'Settlement Type',
		'#options' => _utc_get_payment_map(),
		'#required' => true,
	);

	$form['settlement']['remarks'] = array(
		'#type' => 'textarea',
		'#title' => 'Remarks / Check Date'
	);

	$form['settlement']['submit'] = array(
		'#type' => 'submit',
		'#value' => 'Create Settlement'
	);

	return $form;
}

function utc_perform_settlement_form_validate($form, &$form_state) {
	$values = $form_state['values'];

	//Take care of floating point precision issue to be very accurate.
	$amount = number_format($values['settlement']['amount'], 2, '.', '')*100;
	$left = number_format($values['amount_left'], 2, '.', '')*100;

	if($amount > $left) {
		form_set_error('settlement][amount', "Please input an amount less than the remaining balance of $".number_format($values['amount_left'], 2).".");
	}
}

function utc_perform_settlement_form_submit($form, &$form_state) {
	$values = $form_state['values'];

	//Take care of floating point precision issue to be very accurate.
	$amount = number_format($values['settlement']['amount'], 2, '.', '')*100;
	$left = number_format($values['amount_left'], 2, '.', '')*100;

	if($amount == $left) {
		db_query("UPDATE {user_term_credits_usages} SET settled = '1' WHERE cuid = '%d'", $values['cuid']);
	}

	db_query("INSERT INTO {user_term_credits_settlements}
		(cuid, settlement_amount, payment_type, remarks, settlement_date)
		VALUES
		('%d', '%s', '%s', '%s', '%s')", $values['cuid'], $values['settlement']['amount'], $values['settlement']['type'], $values['settlement']['remarks'], time());

	drupal_set_message('Successfully created settlement.');

	if($amount == $left) {
		drupal_goto("user/".$values['uid']."/term-credits");
	}
}
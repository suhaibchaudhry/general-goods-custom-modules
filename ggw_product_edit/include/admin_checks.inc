<?php
function ggw_duplicate_empty_checkouts() {
	$cash_log_table = 'pos_api_expose_cash_log';
	$check_log_table = 'pos_api_expose_check_log';
	$mo_log_table = 'pos_api_expose_money_order_log';
	$cc_log_table = 'pos_api_expose_credit_card_log';
	$transac_log_table = 'pos_api_expose_transaction_log';

	$transacs = db_query("SELECT tl.credit_usage_id, tl.cash_pid, tl.rma_pid, tl.cc_pid, tl.check_pid, tl.mo_pid, tl.amount, tl.pid, tl.cuid, tl.uid, tl.timestamp, tl.register_id, tl.ticket_id, cl.amount_paid AS cash_paid, cl.change AS change_made, cl.total AS cash_total, chl.paid_amount AS check_payment, chl.check_number, chl.post_dated AS check_post_dated, chl.cash_date AS check_cash_date, ccl.amount_paid AS cc_amount, mo.paid_amount AS mo_paid_amount, mo.reference AS mo_ref, cu.credit_amount AS ar_payment_amount, ccl.transaction_id FROM {".$transac_log_table."} tl
	LEFT JOIN {".$cash_log_table."} cl ON tl.cash_pid = cl.pid
	LEFT JOIN {".$cc_log_table."} ccl ON tl.cc_pid = ccl.pid
	LEFT JOIN {".$check_log_table."} chl ON tl.check_pid = chl.pid
	LEFT JOIN {".$mo_log_table."} mo ON tl.mo_pid = mo.pid
	LEFT JOIN {user_term_credits_usages} cu ON tl.credit_usage_id = cu.cuid
	WHERE tl.amount = '0' AND tl.ticket_id > 0");

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
	}

	return theme('table', $header, $rows);
}

function ggw_duplicate_transactions() {
	//$transacs = db_query("SELECT ");

	$cash_log_table = 'pos_api_expose_cash_log';
	$check_log_table = 'pos_api_expose_check_log';
	$mo_log_table = 'pos_api_expose_money_order_log';
	$cc_log_table = 'pos_api_expose_credit_card_log';
	$transac_log_table = 'pos_api_expose_transaction_log';

	$transacs = db_query("SELECT tl.credit_usage_id, tl.cash_pid, tl.rma_pid, tl.cc_pid, tl.check_pid, tl.mo_pid, tl.amount, tl.pid, tl.cuid, tl.uid, tl.timestamp, tl.register_id, tl.ticket_id, cl.amount_paid AS cash_paid, cl.change AS change_made, cl.total AS cash_total, chl.paid_amount AS check_payment, chl.check_number, chl.post_dated AS check_post_dated, chl.cash_date AS check_cash_date, ccl.amount_paid AS cc_amount, mo.paid_amount AS mo_paid_amount, mo.reference AS mo_ref, cu.credit_amount AS ar_payment_amount, ccl.transaction_id FROM {".$transac_log_table."} tl
	LEFT JOIN {".$cash_log_table."} cl ON tl.cash_pid = cl.pid
	LEFT JOIN {".$cc_log_table."} ccl ON tl.cc_pid = ccl.pid
	LEFT JOIN {".$check_log_table."} chl ON tl.check_pid = chl.pid
	LEFT JOIN {".$mo_log_table."} mo ON tl.mo_pid = mo.pid
	LEFT JOIN {user_term_credits_usages} cu ON tl.credit_usage_id = cu.cuid
	WHERE tl.ticket_id > 0 AND tl.ticket_id IN (SELECT tl2.ticket_id FROM pos_api_expose_transaction_log tl2 GROUP BY tl2.ticket_id HAVING COUNT(*) > 1)");

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

	}

	return theme('table', $header, $rows);
}

function ggw_inconsitent_tickets() {
	$tickets = db_query("SELECT cu.cuid, cu.uid, cu.ticket_id, cu.amount, o.order_total, cl.amount_paid, cl.change, cl.total, o.created, o.modified
		FROM pos_api_expose_transaction_log cu
		INNER JOIN uc_orders o ON cu.ticket_id = o.order_id
		LEFT JOIN pos_api_expose_cash_log cl ON cu.cash_pid = cl.pid
		WHERE ABS(cu.amount-o.order_total) > '0.01' OR (o.uid = '0' AND cu.cuid <> '0')");

	$header = array('Customer', 'Employee', 'Ticket #', 'Closed Amount', 'Current Invoice Amount', 'Cash Paid', 'Change Made', 'Ticket Closing Amount', 'Date Created', 'Date Modified');
	$rows = array();
	while($ticket = db_fetch_object($tickets)) {
		$rows[] = array(
			_ggw_state_reporting_cust_name($ticket->cuid),
			_ggw_state_reporting_cust_name($ticket->uid),
			l($ticket->ticket_id, 'admin/store/orders/'.$ticket->ticket_id),
			uc_currency_format($ticket->amount),
			uc_currency_format($ticket->order_total),
			uc_currency_format($ticket->amount_paid),
			uc_currency_format($ticket->change),
			uc_currency_format($ticket->total),
			date('n/j/Y g:i A', $ticket->created),
			date('n/j/Y g:i A', $ticket->modified)
		);
	}

	return theme('table', $header, $rows);
}

function ggw_product_tickets_view_dp() {
	$tickets = db_query("SELECT order_id, uid FROM uc_orders WHERE billing_zone = '1' AND order_id NOT IN (SELECT ticket_id FROM pos_api_expose_transaction_log_dp WHERE 1)");

	$header = array('Ticket Id', 'Customer', 'Print');
	$rows = array();
	while($ticket = db_fetch_object($tickets)) {
		$rows[] = array(
			$ticket->order_id,
			_ggw_state_reporting_cust_name($ticket->uid),
			l('Print', 'admin/invoice/print/'.$ticket->order_id)
		);
	}

	return theme('table', $header, $rows);
}

function ggw_open_tickets_log() {
	$tickets = db_query("
	SELECT tl.ticket_id, u.uid AS euid, u.name, uc.uid, ucc.name AS cname, uc.created, tl.`timestamp` FROM pos_api_expose_ticket_log tl
	INNER JOIN uc_orders uc ON tl.ticket_id = uc.order_id
	INNER JOIN users u ON tl.uid = u.uid
	LEFT JOIN users ucc ON uc.uid = ucc.uid
	WHERE uc.order_status = 'pos_in_progress' AND (ABS(tl.`timestamp`- uc.created) >= 0) AND (ABS(tl.`timestamp` - uc.created) <= 5)
	ORDER BY tl.ticket_id DESC");

	$header = array('Ticket ID', 'Employee', 'Customer', 'Date Created');
	$rows = array();
	while($row = db_fetch_object($tickets)) {
		if($row->uid) {
			$customer = l($row->cname, 'user/'.$row->uid);
		} else {
			$customer = 'Walk-in Customer';
		}

		$rows[] = array(
			l($row->ticket_id, 'admin/store/orders/'.$row->ticket_id),
			l($row->name, 'user/'.$row->euid),
			$customer,
			date("j/m/Y h:ia", $row->created)
		);
	}

	return theme('table', $header, $rows);
}

function ggw_walk_in_tickets_list() {
	$content = '';
	$tickets = db_query("SELECT DISTINCT uo.order_id FROM uc_order_products ucp INNER JOIN uc_orders uo ON ucp.order_id = uo.order_id WHERE ucp.nid IN (SELECT n.nid FROM uc_products up
			INNER JOIN term_node tn ON up.nid = tn.nid
 			INNER JOIN node n ON up.nid = n.nid
 			INNER JOIN content_type_product ctp ON n.vid = ctp.vid
			INNER JOIN ggw_state_reporting_terms sr ON sr.vid = tn.tid
			WHERE setting > 1) AND uo.uid = '0'");

	$header = array('Closed Walk-In Ticket w/ Tobacco');
	$rows = array();
	while($ticket = db_fetch_object($tickets)) {
		$rows[] = array(
			l($ticket->order_id, 'admin/store/orders/'.$ticket->order_id)
		);
	}

	$content .= theme('table', $header, $rows);

	$tickets = db_query("SELECT order_id, uid FROM uc_orders WHERE uid = '0'");

	$header = array('Closed Walk-In Ticket');
	$rows = array();
	while($ticket = db_fetch_object($tickets)) {
		$rows[] = array(
			l($ticket->order_id, 'admin/store/orders/'.$ticket->order_id)
		);
	}

	$content .= theme('table', $header, $rows);

	return $content;
}

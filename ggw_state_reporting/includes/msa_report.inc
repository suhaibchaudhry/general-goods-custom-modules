<?php
function ggw_state_reporting_download_msa() {
	header("Content-Type: text/plain");
	//header('Content-disposition: attachment;filename=weekly_msa_report.txt');

	$company = 'HID14047001TOB  W20121221GENERAL GOODS WHOLESALE         8000 HARWIN, SUITE 200 HOUSTON, TX 77036                                                  TOBACO ID: 93044639                                                                 7137803636     7137801718                                                            000100000002201212261';
	$address = '8000 HARWIN, SUITE 200 HOUSTON, TX 77036';

	$msa_number = '14047001';
	$tobacco_id = '93044639';
	$phone = '7137803636';
	$fax = '7137801718';

	$start_date = strtotime('-1 week');
	$end_date = strtotime('now');

	$start_date_r = date('Ymd', $start_date);
	$end_date_r = date('Ymd', $end_date);

	$report = '';

	//MSA Report Header
	$report .= ggw_state_reporting_msa_cell('HID'.$msa_number.'TOB', 16);
	$report .= ggw_state_reporting_msa_cell('W'.$start_date_r.$company, 41);
	$report .= ggw_state_reporting_msa_cell($address, 90);
	$report .= ggw_state_reporting_msa_cell('TOBACO ID: '.$tobacco_id, 84);
	$report .= ggw_state_reporting_msa_cell($phone, 15);
	$report .= ggw_state_reporting_msa_cell($fax, 70);
	$report .= '000100000002'.$end_date_r.'1'."\n\n";

	$report .= ggw_state_reporting_msa_get_inventory_report();

	print $report;
}

function ggw_state_reporting_msa_get_inventory_report() {
	$sql = "SELECT n.title, up.model FROM uc_products up
			INNER JOIN term_node tn ON up.nid = tn.nid
 			INNER JOIN node n ON up.nid = n.nid
			INNER JOIN ggw_state_reporting_terms sr ON sr.vid = tn.tid
			WHERE setting > 1";

	$content = '';
	$bid = ggw_state_reporting_msa_cell('BID', 5);
	$products = db_query($sql);

	while($product = db_fetch_object($products)) {
		$content .= $bid.ggw_state_reporting_msa_cell($product->model, 26);
		$content .= "\n";
	}

	return $content;
}
<?php
function _pos_search_features_invoice_search() {
	$content = '';
	$content .= '<div class="pos-search-form">'.drupal_get_form('pos_search_features_invoice_form').'</div><div id="pos-search-results"></div>';
	
	print $content;
}

function pos_search_features_invoice_form(&$form_state) {
	$form['invoice_search'] = array(
		'#type' => 'fieldset',
		'#title' => t('Invoice Search'),
		'#collapsible' => true,
		'#attributes' => array('class' => 'pos-search-form')
	);
	
	$form['invoice_search']['invoice_number'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Invoice #'),
		'#description' => t('Search by Invoice Number')
	);
	
	$form['invoice_search']['invoice_date'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Invoice Date'),
		'#description' => t('Search by Invoice Date')
	);

	$form['invoice_search']['account_number'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Account ID'),
		'#description' => t('Search by Account ID')
	);

	$form['invoice_search']['company_name'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Company Name'),
		'#description' => t('Search by Phone Name')
	);

	return $form;
}
<?php
function _pos_search_features_user_search() {
	$content = '';
	$content .= '<div class="pos-search-form">'.drupal_get_form('pos_search_features_user_form').'</div><div id="pos-search-results"></div>';
	
	print $content;
}

function pos_search_features_user_form(&$form_state) {
	$form['user_search'] = array(
		'#type' => 'fieldset',
		'#title' => t('Customer Search'),
		'#collapsible' => true,
		'#attributes' => array('class' => 'pos-search-form')
	);
	
	$form['user_search']['account_number'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Account ID'),
		'#description' => t('Search by Account ID')
	);
	
	$form['user_search']['company_name'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Company Name'),
		'#description' => t('Search by Company Name')
	);
	
	$form['user_search']['phone_number'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Phone Number'),
		'#description' => t('Search by Company Name')
	);
	
	return $form;
}
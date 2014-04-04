<?php
function _pos_search_features_product_search() {
	$content = '';
	$content .= '<div class="pos-search-form">'.drupal_get_form('pos_search_features_product_form').'</div><div id="pos-search-results"></div>';
	
	print $content;
}

function pos_search_features_product_form(&$form_state) {
	$form['product_search'] = array(
		'#type' => 'fieldset',
		'#title' => t('Product Search'),
		'#collapsible' => true,
		'#attributes' => array('class' => 'pos-search-form')
	);
	
	$form['product_search']['item_number'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Barcode'),
		'#description' => t('Search by UPC Barcode')
	);
	
	$form['product_search']['item_description'] = array(
		'#type' => 'textfield',
		'#size' => 20,
		'#title' => t('Product Description'),
		'#description' => t('Search by Product Description')
	);

	$form['product_search']['product_department'] = array(
		'#type' => 'select',
		'#default_value' => 0,
		'#options' => _pos_search_features_product_depts(),
		'#title' => t('Product Department'),
		'#description' => t('Search by Product Department')
	);

	return $form;
}

function _pos_search_features_product_depts() {
	$select = array('All Departments');
	$terms = taxonomy_get_tree(variable_get('ggw_import_department_vid', 2));
	foreach($terms as $term) {
		$select[$term->tid] = $term->name;
	}
	
	return $select;
}
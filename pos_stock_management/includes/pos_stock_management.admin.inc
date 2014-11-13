<?php
/**
 * Form builder for stock settings form.
 *
 * @ingroup forms
 */
function pos_stock_management_settings_form() {
  $form['psm_vendor_role'] = array(
  	'#type' => 'select',
	'#title' => t('Vendor Role'),
	'#options' => user_roles(true),
	'#default_value' => variable_get('psm_vendor_role', 2),
  );

  return system_settings_form($form);
}

/**
 * Form builder for product stock edit form.
 *
 * @see theme_uc_stock_edit_form()
 * @see uc_stock_edit_form_submit()
 * @ingroup forms
 */
function pos_stock_management_edit_form($form_state, $node) {
  drupal_add_css(drupal_get_path('module', 'pos_stock_management').'/css/pos_stock_management_form.css');

  $form = array();
  
  $form['nid'] = array(
  	'#type' => 'value',
	'#value' => $node->nid
  );

  $form['new_stock'] = array(
	'#type' => 'fieldset',
	'#title' => t('New Stock Addition'), 
	'#collapsible' => FALSE
  );
  
  $form['new_stock']['vendor'] = array(
	'#type' => 'select',
	'#title' => t('Supplying Vendor'), 
	'#default_value' => 0,
	'#options' => pos_stock_management_vendors($node->nid)
  );
  
  $form['new_stock']['quantity_purchased'] = array(
  	'#type' => 'textfield',
	'#title' => t('Quantity Purchased'), 
	'#size' => 9,
	'#maxlength' => 9,
	'#required' => TRUE
  );

  $form['new_stock']['unit_cost'] = array(
  	'#type' => 'textfield', 
	'#title' => t('Unit Cost'), 
	'#size' => 20,
	'#maxlength' => 128,
	'#required' => FALSE
  );
  
  $form['new_stock']['stock_comment'] = array(
  	'#type' => 'textarea',
	'#title' => t('Stock Comment')
  );

  $form['new_stock']['reset'] = array(
  	'#type' => 'checkbox',
  	'#default_value' => 0,
  	'#title' => t('Reset Stock')
  );
  
  $form['new_stock']['add'] = array(
  	'#type' => 'submit',
	'#value' => t('Add To Stock')
  );
  
  $form['stock_history'] = array(
	'#type' => 'fieldset',
	'#title' => t('Stock Status and History'), 
	'#collapsible' => FALSE
  );
  
  $form['stock_history']['purchase_table'] = array(
  	'#type' => 'markup',
	'#value' => pos_stock_management_history($node->nid)
  );

  return $form;
}

/**
 * Form submission handler for uc_stock_edit_form().
 *
 * @see uc_stock_edit_form()
 * @see theme_uc_stock_edit_form()
 */
function pos_stock_management_edit_form_submit($form, &$form_state) {
	db_query("INSERT INTO {pos_in_stock_management} (product_nid, stock_adjustment, vendor_uid, unit_cost, stock_comment, reset, `timestamp`) VALUES (%d, %d, %d, '%s', '%s', %d, '%s')", $form_state['values']['nid'], $form_state['values']['quantity_purchased'], $form_state['values']['vendor'], $form_state['values']['unit_cost'], $form_state['values']['stock_comment'], $form_state['values']['reset'], time());

	drupal_set_message(t('Stock adjustment added.'));
}

function pos_stock_management_edit_form_validate($form, &$form_state) {
	$form_state['values']['unit_cost'];
	$form_state['values']['quantity_purchased'];
	
	if(!is_numeric($form_state['values']['quantity_purchased'])) {
		form_set_error('quantity_purchased', 'Quantity purchased must be a number.');
	}
	
	if(!is_numeric($form_state['values']['unit_cost'])) {
		form_set_error('unit_cost', 'Unit Cost must be a numeric value.');
	}
}

function pos_stock_management_purchase_orders() {
	$content = '';
	$pos = db_query("SELECT * FROM {pos_stock_management_po} psmpo ORDER BY psmpo.changed DESC");
	$header = array('Purchase Order Ref', 'Created Date', 'Last Changed', '');
	$data = array();
	while($po = db_fetch_object($pos)) {
		$data[] = array(l($po->po_ref, 'admin/store/purchase-orders/edit/'. $po->po_id), date('m/d/y g:i A', $po->created), date('m/d/y g:i A', $po->changed), l('Edit', 'admin/store/purchase-orders/edit/'. $po->po_id)." ". l('Delete', 'admin/store/purchase-orders/delete/'. $po->po_id));
	}
	
	$content .= theme('table', $header, $data);
	$content .= drupal_get_form('pos_stock_management_new_po_form');
	return $content;
}

function pos_stock_management_new_po_form($form_state) {
	drupal_add_css(drupal_get_path('module', 'pos_stock_management').'/css/pos_stock_maagement_po.css');

	$form['new_po'] = array(
		'#type' => 'fieldset',
		'#title' => t('Create New Purchase Order')
	);

	$form['new_po']['po_ref'] = array(
		'#type' => 'textfield',
		'#title' => t('Purchase Order Reference'),
		'#size' => 60,
		'#maxlength' => 250,
		'#required' => TRUE
	);
	
	$form['new_po']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Create Purchase Order'),
		'#attributes' => array('class' => 'purchase_order_submit')
	);
	
	return $form;
}

function pos_stock_management_new_po_form_submit($form, &$form_state) {
	db_query("INSERT INTO {pos_stock_management_po} (po_ref, created, changed) VALUES ('%s', %d, %d)", $form_state['values']['po_ref'], time(), time());
	
	drupal_goto('admin/store/purchase-orders/edit/'.db_last_insert_id('pos_stock_management_po', 'po_id'));
}

function pos_stock_management_purchase_order_delete($form_state, $po_id) {
	$form = array(
		'po_id' => array(
			'#type' => 'value',
			'#value' => $po_id
		)
	);

	return confirm_form($form, 'Are you sure you want to delete the purchase order referenced as, '.db_result(db_query("SELECT po_ref FROM {pos_stock_management_po} WHERE po_id = %d", $po_id)).'?', 'admin/store/purchase-orders', 'Any items already sent to stock cannot be reversed, however any items not yet sent to stocks will be lost.', 'Delete Purchase Order', 'Cancel');
}

function pos_stock_management_purchase_order_delete_submit($form, &$form_state) {
	db_query('DELETE FROM {pos_stock_management_po} WHERE po_id = %d', $form_state['values']['po_id']);
	db_query('DELETE FROM {pos_stock_management_po_items} WHERE po_id = %d', $form_state['values']['po_id']);
	
	drupal_goto('admin/store/purchase-orders');
}

function pos_stock_management_purchase_order_edit($po_id) {
	drupal_add_css(drupal_get_path('module', 'jquery_ui').'/jquery.ui/themes/smoothness/jquery-ui-1.7.3.custom.css');
	jquery_ui_add(array('ui.dialog', 'ui.slider', 'effects.drop', 'ui.draggable'));
	
	drupal_add_css(drupal_get_path('module', 'pos_search_features').'/css/pos_dialog_init.css');
	drupal_add_css(drupal_get_path('module', 'pos_search_features').'/css/pos_search_user.css');
	drupal_add_css(drupal_get_path('module', 'pos_stock_management').'/css/pos_stock_magement_edit.css');
	
	drupal_add_js(drupal_get_path('module', 'pos_search_features').'/js/pos_dialog_init.js');
	drupal_add_js(drupal_get_path('module', 'pos_stock_management').'/js/pos_stock_management_edit.js');
	
	return drupal_get_form('pos_stock_management_pruchase_order_edit_form', $po_id);
}

function pos_stock_management_pruchase_order_edit_form($form_state, $po_id) {
	$form = array();
	$purchase_order = db_fetch_object(db_query("SELECT * FROM {pos_stock_management_po} WHERE po_id = %d", $po_id));
	
	$form['po_id'] = array(
		'#type' => 'value',
		'#value' => $po_id
	);
	
	$form['po_name'] = array(
		'#title' => 'Purchase Order Reference',
		'#type' => 'textfield',
		'#default_value' => $purchase_order->po_ref
	);
	
	$form['products'] = array(
		'#type' => 'fieldset',
		'#title' => 'Add Items'
	);

	$form['products']['items'] = array(
		'#type' => 'markup',
		'#value' => '<table class="po-items"><tr><th>Barcode</th><th>Description</th><th>Stock Adjustment</th><th>Unit Cost</th><th>Vendor</th><th>Delete</th></tr>'.pos_stock_management_prod_po_lineitem_saved($po_id).'</table>'
	);

	if($purchase_order->submitted != 1) {
		$form['products']['add_items'] = array(
			'#value' => 'Add Item',
			'#type' => 'button',
			'#attributes' => array('onclick' => 'return false;')
		);

		$form['po_save'] = array(
			'#value' => 'Save Purchase Order',
			'#type' => 'submit',
			'#submit' => array('pos_stock_management_pruchase_order_edit_save'),
		);
		
		$form['po_send'] = array(
			'#value' => 'Send to Inventory',
			'#type' => 'submit',
			'#submit' => array('pos_stock_management_pruchase_order_edit_substock')
		);
	}

	return $form;
}

function pos_stock_management_pruchase_order_edit_substock($form, &$form_state) {
	pos_stock_management_pruchase_order_edit_save($form, $form_state);
	$po_id = $form_state['values']['po_id'];
	$po_ref = db_result(db_query("SELECT po_ref FROM {pos_stock_management_po} WHERE po_id = %d", $po_id));
	$po_items = db_query("SELECT * FROM {pos_stock_management_po_items} WHERE po_id = %d ORDER BY po_item_id DESC", $po_id);
	while($po_item = db_fetch_object($po_items)) {
		db_query("INSERT INTO {pos_in_stock_management} (product_nid, stock_adjustment, vendor_uid, unit_cost, stock_comment, `timestamp`) VALUES (%d, %d, %d, %f, '%s', '%s')", $po_item->product_nid, $po_item->stock_adjustment, $po_item->vendor_uid, $po_item->unit_cost, $po_ref, time());
	}
	
	db_query("UPDATE {pos_stock_management_po} SET submitted = 1 WHERE po_id = %d", $po_id);
	
	drupal_set_message('Purchase order was successfully submitted to stock. It can no longer be modified.');
}

function pos_stock_management_pruchase_order_edit_save($form, &$form_state) {
	db_query("UPDATE {pos_stock_management_po} SET po_ref = '%s', changed = %d WHERE po_id = %d", $form_state['values']['po_name'], time(), $form_state['values']['po_id']);

	db_query("DELETE FROM {pos_stock_management_po_items} WHERE po_id = %d", $form_state['values']['po_id']);
	if(is_array($form_state['clicked_button']['#post']['product_nid'])) {
		foreach($form_state['clicked_button']['#post']['product_nid'] as $key => $product_nid) {
			if(!isset($form_state['clicked_button']['#post']['delete_poli'][$key])) {
				db_query("INSERT INTO {pos_stock_management_po_items} (po_id, product_nid, stock_adjustment, unit_cost, vendor_uid) VALUES (%d, %d, %d, %f, %d)", $form_state['values']['po_id'], $product_nid, $form_state['clicked_button']['#post']['po_qty'][$key], $form_state['clicked_button']['#post']['po_price'][$key], $form_state['clicked_button']['#post']['vendors'][$key]);
			}
		}
	}

	drupal_set_message('Purchase Order Saved');
}

function pos_stock_management_prod_po_lineitem_saved($po_id) {
	$content = '';
	$po_items = db_query("SELECT * FROM {pos_stock_management_po_items} WHERE po_id = %d ORDER BY po_item_id DESC", $po_id);
	while($po_item = db_fetch_object($po_items)) {
		$content .= pos_stock_management_prod_po_lineitem_defaults($po_item->product_nid, $po_item->stock_adjustment, $po_item->unit_cost, $po_item->vendor_uid);
	}

	return $content;
}

function pos_stock_management_prod_po_lineitem_defaults($nid, $qty, $unit_cost, $vendor_uid) {
	$barcode = db_result(db_query("SELECT model FROM {uc_products} WHERE nid = '%s'", $nid));
	$desc = db_result(db_query("SELECT title FROM {node} WHERE nid = '%d'", $nid));
	$vendors = pos_stock_management_prod_vendor_opts($nid, $vendor_uid);

	$content = '<tr>';
	$content .= '<td>'.$barcode.'</td>';
	$content .= '<td>'.l($desc, 'node/'.$nid.'/edit/pos-stock-management', array('attributes' => array('target' => '_new'))).'</td>';
	$content .= '<td><input type="hidden" name="product_nid[]" value="'.$nid.'" /><input value="'.$qty.'" size="9" class="form-text" name="po_qty[]" /></td>';
	$content .= '<td><span>$</span><input class="form-text" name="po_price[]" value="'.format_number($unit_cost, 2).'" /></td>';
	$content .= '<td>'.$vendors.'</td>';
	$content .= '<td><input type="checkbox" name="delete_poli[]" value="1" /></td>';
	$content .= '</tr>';
	
	return $content;
}

function pos_stock_management_prod_po_lineitem($barcode) {
	$nid = db_result(db_query("SELECT nid FROM {uc_products} WHERE model = '%s'", $barcode));
	$desc = db_result(db_query("SELECT title FROM {node} WHERE nid = '%d'", $nid));
	$vendors = pos_stock_management_prod_vendor_opts($nid);

	$content = '<tr>';
	$content .= '<td>'.$barcode.'</td>';
	$content .= '<td>'.l($desc, 'node/'.$nid.'/edit/pos-stock-management', array('attributes' => array('target' => '_new'))).'</td>';
	$content .= '<td><input type="hidden" name="product_nid[]" value="'.$nid.'" /><input size="9" class="form-text" name="po_qty[]" /></td>';
	$content .= '<td><span>$</span><input class="form-text" name="po_price[]" /></td>';
	$content .= '<td>'.$vendors.'</td>';
	$content .= '<td><input type="checkbox" name="delete_poli[]" value="1" /></td>';
	$content .= '</tr>';
	
	print $content;
}

function pos_stock_management_prod_vendor_opts($nid, $default = FALSE) {
	$vendors = pos_stock_management_vendors($nid);

	$content = '<select name="vendors[]" class="form-select">';
	foreach($vendors as $v_uid => $vendor) {
		if($default !== FALSE && $v_uid == $default) {
			$selected = ' selected="selected"';
		} else {
			$selected = '';
		}
		$content .= '<option value="'.$v_uid.'"'.$selected.'>'.$vendor.'</option>';
	}
	$content .= '</select>';
	
	return $content;
}
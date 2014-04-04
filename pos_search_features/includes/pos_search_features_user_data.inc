<?php
function _pos_search_features_user_search_data() {
	$char_limit = 3;
	$account_id = $_POST['account_number'];
	$company_name = $_POST['company_name'];
	$phone_number = $_POST['phone_number'];
	$sorts = array();
	
	if(_pos_search_features_input_size_validate($char_limit, $account_id, $company_name, $phone_number)) {
		print '<p>Please type in atleast '.$char_limit.' characters for atleast one search criteria to view results.</p>';
	} else {
		$wheres = array();
		
		$wheres[] = "n.type = 'profile'";
		
		_pos_search_features_construct_like_wheres($wheres, array(
			'u.name' => $account_id,
			'n.title' => $company_name,
			'p.field_profile_company_phone_number' => $phone_number
		));
		
		$sorts[] = 'n.title ASC';

		if(!empty($wheres)) {
			$fieldMap = array(
				'{users} u' => array('u.uid', 'u.name'),
				'{node} n ON u.uid = n.uid' => array('n.title'),
				'{content_type_profile} p ON n.nid = p.nid' => array('p.field_profile_company_phone_number')
			);
			
			$query = _pos_search_features_construct_query($fieldMap, $wheres, $sorts);
		
			$data = array();
			$customers = db_query($query);
			$header = array('Account #', 'Company Name', 'Phone Number');
			while($customer = db_fetch_object($customers)) {
				$data[] = array('<a href="#" onclick="return false;" class="pos-search-select" rel="'.$customer->uid.'ID'.'">'.$customer->name.'</a>', $customer->title, $customer->field_profile_company_phone_number);
			}

			print theme('table', $header, $data);
		}
	}
}
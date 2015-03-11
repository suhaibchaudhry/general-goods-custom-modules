<?php
function ggw_profile_add_new_user_ins() {
	$failed_response = array(
		'status' => false,
		'error' => 'Employee token has expired.'
	);

	$request = json_decode($_POST['request']);
	$uid = _pos_api_expose_uid_from_token($request->token);

	$companyName = trim($request->companyName);
	$firstName = trim($request->firstName);
    $lastName = trim($request->lastName);
    $accountId = trim($request->accountId);
    $phone = _ggw_profile_edit_formatNumber(trim($request->phone));
    $email = trim($request->email);
    $password = md5(trim($request->password));
    $taxId = trim($request->taxId);
    $tobacco = trim($request->tobacco);
    $tremarks = trim($request->tremarks);
    $tdate = trim($request->tdate);
    $st_one = trim($request->st_one);
    $st_two = trim($request->st_two);
    $city = trim($request->city);
    $state = trim($request->state);
    $zip = trim($request->zip);
    $fax = _ggw_profile_edit_formatNumber(trim($request->fax));
    $customer_role = trim($request->customer_role);
    $pricing_role = trim($request->pricing_role);
    $tax_type = trim($request->tax_type);

	if($uid) {
		$userCount = db_result(db_query("SELECT COUNT(*) FROM users WHERE name = '%s'", $accountId));
		if($userCount == 0) {
			if(empty($email)) {
				$email = $accountId."@general-goods.com";
			}

			$timestamp = time();

			db_query("INSERT INTO users (name, pass, mail, mode, sort, threshold, signature_format, created, access, login, status, timezone, data, timezone_name) VALUES('%s', '%s', '%s', '0', '0', '0', '0', '%s', '%s', '%s', '1', '-2600', NULL, 'America/Chicago');", $accountId, $password, $email, $timestamp, $timestamp, $timestamp);

			$uid = db_last_insert_id('users', 'uid');
			if(!empty($customer_role)) {
				db_query("INSERT INTO users_roles (uid, rid) VALUES('%d', '%d');", $uid, $customer_role);
			} else {
				db_query("INSERT INTO users_roles (uid, rid) VALUES('%d', '9');", $uid);
			}

			if(!empty($pricing_role)) {
				db_query("INSERT INTO users_roles (uid, rid) VALUES('%d', '%d');", $uid, $pricing_role);
			}

			db_query("INSERT INTO node_revisions (uid, title, timestamp, format) VALUES('%d', '%s', '%s', '0');", $uid, $companyName, $timestamp);

			$vid = db_last_insert_id('node_revisions', 'vid');

			db_query("INSERT INTO node (vid, type, title, uid, status, created, changed, comment, promote, moderate, sticky, tnid, translate) VALUES('%d', 'profile', '%s', '%d', '1', '%s', '%s', '0', '0', '0', '-100', '0', '0');", $vid, $companyName, $uid, $timestamp, $timestamp);

			$nid = db_last_insert_id('node', 'nid');
			db_query("UPDATE node_revisions SET nid = '%d' WHERE vid = '%d'", $nid, $vid);


			db_query("INSERT INTO content_type_profile (vid, nid, field_contact_remarks_value, field_last_name_value, field_profile_phone_number, field_profile_phone_country_codes, field_profile_phone_extension, field_profile_address_type, field_profile_address_name, field_profile_address_street1, field_profile_address_street2, field_profile_address_zip, field_profile_address_city, field_profile_address_state, field_profile_address_country, field_profile_fax_number, field_profile_fax_country_codes, field_profile_fax_extension, field_company_address_type, field_company_address_name, field_company_address_street1, field_company_address_street2, field_company_address_zip, field_company_address_city, field_company_address_state, field_company_address_country, field_profile_company_phone_number, field_profile_company_phone_country_codes, field_profile_company_phone_extension, field_profile_company_fax_number, field_profile_company_fax_country_codes, field_profile_company_fax_extension, field_tobacco_permit_id_value, field_profile_company_website_value, field_profile_employee_ssn_value, field_profile_employee_joindate_value, field_profile_first_name_value, field_profile_hourly_rate_value, field_tobacco_permit_remarks_value, field_tobacco_permit_exp_value, field_tax_id_value, field_shipping_address_type, field_shipping_address_name, field_shipping_address_street1, field_shipping_address_street2, field_shipping_address_zip, field_shipping_address_city, field_shipping_address_state, field_shipping_address_country) VALUES ('%d', '%d', '', '', '%s', 'us', '', NULL, NULL, '%s', '%s', '%s', '%s', '%s', NULL, '%s', 'us', '', NULL, NULL, '%s', '%s', '%s', '%s', '%s', NULL, '%s', 'us', '', '%s', 'us', '', '%s', '', NULL, NULL, NULL, NULL, '%s', '%s', '%s', NULL, NULL, '%s', '%s', '%s', '%s', '%s', 'us')", $vid, $nid, $phone, $st_one, $st_two, $zip, $city, $state, $fax, $st_one, $st_two, $zip, $city, $state, $phone, $fax, $tobacco, $tremarks, $tdate, $taxId, $st_one, $st_two, $zip, $city, $state);

			if($tax_type == "W") {
				db_query("INSERT INTO ggw_state_reporting_taxes (uid, retail_tax, wholesale_tax) VALUES ('%d', 'N', 'Y')", $uid);
			} else {
				db_query("INSERT INTO ggw_state_reporting_taxes (uid, retail_tax, wholesale_tax) VALUES ('%d', 'Y', 'N')", $uid);
			}

			pos_api_expose_respond(array(
				'status' => true,
				'error' => 'Customer account found.'
			));
		} else {
			pos_api_expose_respond(array(
				'status' => false,
				'error' => 'Customer account number already exists.'
			));
		}
	} else {
		pos_api_expose_respond($failed_response);
	}
}

function _ggw_profile_edit_formatNumber($phoneNumber) {
	return preg_replace("/[^0-9]+/", "", $phoneNumber);
}
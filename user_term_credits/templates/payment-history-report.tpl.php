<?php
if($uid) {
  $profile = content_profile_load('profile', $uid);
}
?>
<!DOCTYPE HTML>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
  <link rel="stylesheet" type="text/css" href="<?php print base_path() . drupal_get_path('theme', 'ggw_backend'); ?>/css/invoice.css" media="all" />
  <title><?php print $profile->title ?> (<?php print $profile->name ?>)</title>
  <style type="text/css" media="all">
  body {
    -webkit-print-color-adjust:exact;
  }
  table td.numeric-item {
    text-align: right;
  }
  table td.qty-item {
    text-align: center;
  }
  table th.left-item {
    text-align: left;
  }
  .cust-address {
  	width: 100%;
  }
  .cust-address span.label {
  	margin: 0 0 1em 0;
  }
  table tr td {
  	text-align: center;
  }
  .cust-address {
    margin: 0 0 2em;
  }
  table.payment-history-table {
  	width: 100%;
  }
  .container-inline-date {
  	float: left;
  	margin: 0 0 50px;
  }

  a {
  	text-decoration: none;
  	color: #000;
  }
  a.print {
  	clear: both;
  	display: block;
  	font-size: 20px;
  	margin: bottom;
  }

  @media print {
	  input.form-submit {
	  	display: none;
	  	visibility: hidden;
	  }
	  a.reset-button {
	  	display: none;
	  	visibility: hidden;
	  }
	  a.print {
	  	display: none;
	  	visibility: hidden;
	  }
  }

  @media screen {
	body {
	font-size: 1.5em;
	}
  }
  </style>
  <script type="text/javascript">
  function printAndClose() {
    window.print();
    //setTimeout(function(){window.close();}, 1);
    return false;
  }
  </script>
</head>
<body>
  <div class="invoice">
    <div class="header">
      <div class="logo column"></div>
      <div class="address column">
        <p>8000 Harwin dr. Suite #200 Houston, TX 77036</p>
        <p><span class="label">Email: </span>info@general-goods.com</p>
        <p><span class="label">Website: </span>info@general-goods.com</p>
        <p><span class="label">Phone: </span>713-780-3636 <span class="label">Fax: </span>713-780-1718</p>
        <?php if($profile->field_tobacco_permit_id[0]['value']) : ?><p><span class="label">Tobacco ID: </span>93044639</p><?php endif; ?>
      </div>
      <div class="invoice-info column">
        <p class="invoiceid" style="font-size: 1.5em;"><span class="label"><?php print $report_title ?></span></p>
        <p class="invoicedate"><span class="label">Print Date:</span> <span class="value"><?php echo date("n/j/Y g:ia", time()); ?></span></p>
      </div>
    </div>

    <div class="customer-info box">
      <div class="header-line"></div>
      <div class="billing-address cust-address">
        <span class="label">Customer:</span>
        <span class="value">
          <?php if($uid) { ?>
          <?php print $profile->title ?> (<?php print $profile->name ?>)<br />
          <?php print $profile->field_contact_remarks[0]['value'] ?><br />
          <?php print $profile->field_company_address[0]['street1'] ?><br />
          <?php print $profile->field_company_address[0]['city'] ?>, <?php print $profile->field_company_address[0]['state'] ?> <?php print $profile->field_company_address[0]['zip'] ?><br />
          <?php } else { ?>
            Walk-in Customer
          <?php } ?>
        </span>
      </div>
    </div>
<?php print $content ?>
<a href="#" class="print" onclick="javascript:return printAndClose();">Print</a>
 </div>
</body>
</html>

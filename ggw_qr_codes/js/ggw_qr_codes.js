$('div.item-scans .item').live('click', function(event) {
		$(this).find('.title a').click(function(event) {
			//event.stopPropagation();
		});

		//event.stopPropagation();
		event.preventDefault();
		window.open($(this).find('.title a').attr('href'), '_blank');
	});
	
Drupal.behaviors.ggw_qr_codes = function(context) {
	var $input = $('input.barcode-input', context);
	$input.focus();
	$(document).click(function() {
		$input.focus();
	});

	var $form = $('form#ggw-qr-codes-gen-form div', context);

	$input.keyup(function (e) {
    	if (e.keyCode == 13) {
    		var $active = $('.changeUnitBarcode', context);
			var barcode = $input.val();
    	    $input.val('');
    	    if ($active.length > 0) {
    	    	var nid = $active.attr('data-nid');
    	    	//console.log(nid);
    	    	$.post(Drupal.settings.basePath+'admin/store/qr-codes/barcode-update', {nid: nid, barcode: barcode}, function(data) {
    	    		if(data.status) {
    	    			$.jGrowl(data.message);
    	    		}
    	    	});
    	    	$active.removeClass('changeUnitBarcode');
				$active.find('.controls a').text('Change Unit Item Barcode');
    	    } else {
				if($('.item-scans .item[data-barcode="'+barcode.toLowerCase()+'"]').length == 0 && $('.item-scans .item[data-ubarcode="'+barcode.toLowerCase()+'"]').length == 0 && $('.item-scans .item[data-abarcode="'+barcode.toLowerCase()+'"]').length == 0) {
					$.post(Drupal.settings.basePath+'admin/store/qr-codes/item', {item_barcode: barcode}, function(data) {
						//console.log(data);
						if(data.status) {
							$data = $('<div class="item"></div>');
							//$data.data('nid', data.nid);
							$data.attr('data-barcode', data.barcode.toLowerCase());
							$data.attr('data-ubarcode', data.ubarcode.toLowerCase());
							var barcodes = '<div class="barcode">'+data.barcode+'<br />'+data.ubarcode;
							if(data.addl) {
								barcodes += '</br>'+data.addl+' (Additional Barcode)';
								$data.attr('data-abarcode', data.addl.toLowerCase());
							}
							$data.attr('data-nid', data.nid);
							barcodes += '</div>';
							$data.append(barcodes);
							$data.append('<div class="title"><a href="'+Drupal.settings.basePath+'node/'+data.nid+'" target="_blank">'+data.title+'</a></div>');
							$data.append('<div class="image"><img src="'+data.image+'" alt="" /></div>');

							$data.append('<div class="controls"><a href="#">Change Unit Item Barcode</a></div>');
							var list = $('.item-list', context);
							list.append($data);
							$form.append('<input type="hidden" name="item[]" value="'+data.nid+'" />');
						} else {
							$.jGrowl("Could not find item with barcode: <strong>"+barcode+"</strong>");
						}
					});
				} else {
					$.jGrowl("Item was aleady scanned previously.");
				}
			}
    	}
	});

	var controls = $('.item-list', context).find('.controls a');
	controls.live('click', function(event) {
		event.stopPropagation();
		event.preventDefault();
		var $this = $(this);
		var $item = $this.parent().parent();
		if($item.is('.changeUnitBarcode')) {
			$this.text('Change Unit Item Barcode');
			$item.removeClass('changeUnitBarcode');
		} else {
			var selected = $('.changeUnitBarcode', context);
			selected.removeClass('changeUnitBarcode');
			selected.find('.controls a').text('Change Unit Item Barcode');
			$this.text('Cancel');
			$item.addClass('changeUnitBarcode');
		}
		$input.focus();
	});
}
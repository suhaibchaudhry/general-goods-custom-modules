Drupal.behaviors.barcode_reader = function(context) {
	//var buffer = "";
	var $upcBox = $('input#edit-model', context);
	if($upcBox.length) {
		$upcBox.focus();
		var clear_flag = $upcBox.val().length;
	
		$('#node-form').keypress(function(e) {
			if((e.keyCode || e.which) == 13) {
				e.preventDefault();
				e.stopPropagation();
			}
		});
		
		var $field = $('#edit-title, #edit-field-prod-packing-0-value');
		$field.wrap('<div class="autofill-field"></div>');
		$('div.autofill-field').append('<a href="#" class="reload">Reload</a><a href="#" class="clear">Clear</a>');
		
		$('div.autofill-field a.clear').click(function(e) {
			e.preventDefault();
			e.stopPropagation();
			$(this).siblings('input').val('');
			$upcBox.focus();
		});
		
		$('div.autofill-field a.reload').click(function(e) {
			var e = jQuery.Event("keypress");
			e.keyCode = 13;
			e.which = 13;
			$upcBox.trigger(e);
			$upcBox.focus();
		});
	
		$upcBox.keypress(function(e) {
			e.preventDefault();
			e.stopPropagation();
	
			if((e.keyCode || e.which) == 13) {
				//console.log("Enter Pressed");
				clear_flag = true;
				
				$.ajax({
					type: "POST",
					url: Drupal.settings.basePath+'admin/upc-database',
					data:{upcCode: this.value},
					dataType: 'json',
					success:  function(product) {
						console.log(product);
						if(!product.error && (product.status == 'success')) {
							var title = $('input#edit-title', context);
							var packing = $('input#edit-field-prod-packing-0-value', context);
							if(title.val() == '') {
								title.val(product.description);
							}
							if(packing.val() == '') {
								packing.val(product.size);
							}
						}
						
						if(product.duplicate) {
							if(confirm('A product with that barcode already exists. Would you like to open it?')) {
								window.open(Drupal.settings.basePath+'node/'+product.nid+'/edit', '_newtab');
							}
						}
					}
				});
				
			} else {
				if(clear_flag) {
					this.value = '';
					clear_flag = false;
				}
				this.value += String.fromCharCode(e.keyCode);
			}
		});
	}
}

Drupal.behaviors.price_total_check = function(context) {
	$('#node-form', context).submit(function(e) {
		var $sell_price = $('#edit-sell-price');
		var sell_price = parseFloat($sell_price.val());
		if($sell_price.val() == '' || isNaN(sell_price) || sell_price <= 0) {
			e.preventDefault();
			e.stopPropagation();
			$sell_price.focus();
			alert('Invalid sell price.');
		}
	});
}
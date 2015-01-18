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

Drupal.behaviors.profitCalc = function(context) {
	var ignoreNonNumeric = function(e) {
	  var charCode = e.keyCode || e.which;
	  var charStr = String.fromCharCode(charCode);

	  var reg = /^\d|\.$/;
	  if(!reg.test(charStr)) {
	    e.preventDefault();
	  }
	};

	var percentageToCostAndSell = function($ele, $input, profit) {
		var percentage = profit.val();
		var cost = $('input#edit-cost').val();
		if(!isNaN(percentage) && !isNaN(cost) && percentage !== '' && cost !== '') {
			var sell_price = (parseFloat(cost)/(100-parseFloat(percentage)))*100;
			$input.val(sell_price.toFixed(2));
		}
	};

	var costAndSellToPercentage = function($ele, $input, init) {
		var sell_price = $input.val();
		var cost = $('input#edit-cost').val();
		var profit = $ele.find('.profitInput');

		if(!isNaN(sell_price) && !isNaN(cost) && sell_price !== '' && cost !== '') {
 			var markup_percentage = 100-((parseFloat(cost)*100)/parseFloat(sell_price));
 			profit.val(markup_percentage.toFixed(2));
 		}

 		if(init) {
 			profit.keypress(ignoreNonNumeric);
 			profit.keyup(function(e) {
 				percentageToCostAndSell($ele, $input, profit);
 			});
 		}
	};

	var attachCalculator = function(ele) {
		var $ele = $(ele);
		var $input = $ele.find('input');
		var $editCost = $('input#edit-cost');
		$input.after('<div class="fieldContainer"></div>');
		var $fieldContainer = $ele.find('.fieldContainer');
		$fieldContainer.append($input);
 		$fieldContainer.append('<input class="profitInput" name="'+$ele.attr('id')+'"/><span class="profitInputSuffix">%</span>');

 		$ele.find('.description').css('clear', 'both');
 		$ele.find('.field-prefix').css('float', 'left');

 		costAndSellToPercentage($ele, $input, true);

 		$input.keypress(ignoreNonNumeric);
 		$input.keyup(function(e) {
 			costAndSellToPercentage($ele, $input, false);
 		});

		$editCost.keypress(ignoreNonNumeric);
		$editCost.keyup(function(e) {
			costAndSellToPercentage($ele, $input, false);
		});
	};

	$('#edit-sell-price-wrapper, #edit-role-prices-12-wrapper, #edit-role-prices-13-wrapper, #edit-role-prices-14-wrapper, #edit-role-prices-15-wrapper', context).each(function(i, e) {
		attachCalculator(e);
	});
}

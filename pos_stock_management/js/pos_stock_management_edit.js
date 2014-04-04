Drupal.behaviors.po_add_items = function() {
	$('input#edit-add-items').click(function () {
		Drupal.posDialogOpen(Drupal.settings.basePath+'?q=admin/store/pos/psf/product-search', 'Add Products', function() {
			Drupal.attachBehaviors($('#pos-dialog').get(0));

			var inputs = $('#pos-dialog fieldset.pos-search-form input, #pos-dialog fieldset.pos-search-form select');

			$('#pos-dialog fieldset.pos-search-form input').bind('keyup', function() {
				formPOChangeHandler(inputs);	
			});
			
			$('#pos-dialog fieldset.pos-search-form select').change(function() {
				formPOChangeHandler(inputs);
			});
		});
	});
}

function formPOChangeHandler(inputs) {
	$('#pos-search-results').html('<div class="dialog-loader"></div>');
	
	var obj = {};
	inputs.each(function() {
		obj[$(this).attr('name')] = $(this).val();
	});

	$.post(Drupal.settings.basePath+'?q=admin/store/pos/psf/product-search/data', obj, function(data) {
		$('#pos-search-results').html(data);
			
		$('#pos-search-results table tr').click(function() {
			var button = $(this).find('a.pos-search-select');
			$.get(Drupal.settings.basePath+'admin/store/purchase-orders/item/'+button.attr('rel'), function(data) {
				$('table.po-items tr:first').after(data);
			});
			Drupal.posDialog.dialog("close");
		});
	});
}
Drupal.behaviors.pos_search_user = function(context) {
	$('.uberpos-button-advance-search', context).click(function() {
		var $this = $(this);
		Drupal.posDialogOpen($this.data('dialog-callback'), $this.text(), function() {
			Drupal.attachBehaviors($('#pos-dialog').get(0));
			var inputs = $('#pos-dialog fieldset.pos-search-form input, #pos-dialog fieldset.pos-search-form select');

			$('#pos-dialog fieldset.pos-search-form input').bind('keyup', function() {
				formChangeHandler($this, inputs);	
			});
			
			$('#pos-dialog fieldset.pos-search-form select').change(function() {
				formChangeHandler($this, inputs);
			});
		});
	});
}

function formChangeHandler($this, inputs) {
	$('#pos-search-results').html('<div class="dialog-loader"></div>');
	
	var obj = {};
	inputs.each(function() {
		obj[$(this).attr('name')] = $(this).val();
	});

	$.post($this.data('ahah-callback'), obj, function(data) {
		$('#pos-search-results').html(data);
			
		$('#pos-search-results table tr').click(function() {
			var button = $(this).find('a.pos-search-select');
			if($this.hasClass('uberpos-button-preclear')) {
				search_uberpos_ajax_execute('CL', function() {
					search_uberpos_ajax_execute(button.attr('rel'));
				});
			} else {
				search_uberpos_ajax_execute(button.attr('rel'));
			}

			Drupal.posDialog.dialog("close");
		});
	});
}

function search_uberpos_ajax_execute(input, callback) {
  //console.log(input);
  var sel = $('table#main-table tr.selected td.product-title').attr('id');

  show_uberpos_throbber();

  $.ajax({
    type: "POST",
    url: Drupal.settings.basePath + '?q=admin/store/pos/ajax',
    data: 'order_id=' + Drupal.settings.uberpos.order_id + '&input=' + escape(input) + '&item=' + sel,
    success: function(data) {
      uberpos_screen_ajax_success(data);
      hide_uberpos_throbber();
	  if(typeof callback == "function") {
	  	callback();
	  }
    }

  });

  return false;
}
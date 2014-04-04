Drupal.behaviors.pos_search_dialog_init = function(context) {
	$('body', context).append('<div id="pos-dialog" />');
	
	Drupal.posDialog = $("#pos-dialog").dialog({
		autoOpen: false,
		show: "drop",
		hide: "drop",
		draggable: true,
		width: 1020,
		height: 489,
		modal: false,
		//Too performance hungry
		/*
		open: function() {
			$('*').disableSelection();	
		},
		close: function() {
			$('*').enableSelection();
		}
		*/
	});

	Drupal.posDialogOpen = function (url, title, callback) {
		Drupal.posDialog.html('<div class="dialog-loader"></div>');
		Drupal.posDialog.dialog("open");
		Drupal.posDialog.dialog("option", "title", title);
		$.get(url,{},function(html) {
			Drupal.posDialog.html(html);
			if(typeof callback == "function") {
				callback();
			}
		});
	}
}
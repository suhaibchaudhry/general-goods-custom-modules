Drupal.behaviors.pos_image_sync_copy = function() {
	$('a.pos_image_sync_copy').bind('click', function(event) {
		event.preventDefault();
		event.stopPropagation();

		var parent = $(this).parent('td');
		var neighbourhood = parent.siblings();
		neighbourhood.find('input[type="text"]').val(parent.find('span.upc').text());
	});
}
Drupal.behaviors.posSignupForm = function(context) {
	$('#edit-pass-wrapper', context).next().css(
		{visibility: 'hidden',
		position: 'absolute'}).next().css({
			clear: 'both'
		}).find('> label').text('Customer Type:');

	$('#edit-user-register-timezone', context).prev().find('.form-item-labeled').css({
		width: '45%',
		float: 'left'
	});

	$('form#user-register', context).submit(function(e) {
		if(!$('input#edit-roles-11').is(':checked') && !$('input#edit-roles-9').is(':checked')) {
			e.preventDefault();
			e.stopPropagation();
			alert("Please select a customer type.");
		}
	});
}
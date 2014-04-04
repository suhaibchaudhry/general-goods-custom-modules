Drupal.behaviors.ggw_mobile_orders_countdown = function(context) {
	countdown(60);
}

function countdown(remaining) {
	if(remaining <= 0) {
		location.reload(true);
	} else {
		document.getElementById('countdown').innerHTML = 'Refresh in: '+remaining+' seconds.';
		setTimeout(function(){ countdown(remaining - 1); }, 1000);	
	}
}
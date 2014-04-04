Drupal.behaviors.conditionalRoles = function(context) {
	$('fieldset.group-company-information, fieldset.group-employee-information', context).hide();
	$('div.form-checkboxes input', context).bind('click', function(e) {
		
		delegateInverses($(this), ['#edit-roles-9', '#edit-roles-10', '#edit-roles-11'], ['#edit-roles-8', '#edit-roles-6', '#edit-roles-3', '#edit-roles-4', '#edit-roles-5'], $('fieldset.group-employee-information', context));
		delegateInverses($(this), ['#edit-roles-8', '#edit-roles-6', '#edit-roles-3', '#edit-roles-4', '#edit-roles-5'], ['#edit-roles-9', '#edit-roles-10', '#edit-roles-11'], $('fieldset.group-company-information', context));
		
		delegateInverses($(this), ['#edit-roles-11'], ['#edit-roles-9']);
		delegateInverses($(this), ['#edit-roles-9'], ['#edit-roles-11']);
		
		checkRole($(this), ['#edit-roles-9', '#edit-roles-10', '#edit-roles-11'], $('fieldset.group-company-information', context));
		checkRole($(this), ['#edit-roles-8', '#edit-roles-6', '#edit-roles-3', '#edit-roles-4', '#edit-roles-5'], $('fieldset.group-employee-information', context));
	});
}

function delegateInverses(clickedRole, selectSet, unselectSet, unselectFieldset) {
	$.each(selectSet, function(index, role) {
		if(clickedRole.is(role)) {
			if(typeof unselectFieldset != "undefined") {
				unselectFieldset.hide();
			}
			for(i in unselectSet) {
				$(unselectSet[i]).attr('checked', false);
			}
		}
	});
}

function checkRole(clickedRole, roleCollection, fieldset) {
	$.each(roleCollection, function(index, role) {
		if(clickedRole.is(role)) {
			if(roleChecked(roleCollection)) {
				fieldset.show();
			} else {
				fieldset.hide();	
			}
		}
	});
}

function roleChecked(roleCollection) {
	for(i in roleCollection) {
		if($(roleCollection[i]).attr('checked')) {
			return true;
		}
	}

	return false;
}
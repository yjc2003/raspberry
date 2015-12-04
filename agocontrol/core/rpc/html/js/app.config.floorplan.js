/**
 * Model class
 * 
 * @returns {floorPlanConfig}
 */
function floorPlanConfig() {
    this.hasNavigation = ko.observable(true);
    this.floorplans = ko.observableArray([]);

    var self = this;

    this.makeEditable = function(row, item) {
	window.setTimeout(function() {
	    $(row).find('td.edit_fp').editable(function(value, settings) {
		var content = {};
		content.floorplan = item.uuid;
		content.uuid = agoController;
		content.command = "setfloorplanname";
		content.name = value;
		sendCommand(content);
		return value;
	    }, {
		data : function(value, settings) {
		    return value;
		},
		onblur : "cancel"
	    });
	}, 1);
    };

    this.deletePlan = function(item, event) {
	var button_yes = $("#confirmDeleteButtons").data("yes");
	var button_no = $("#confirmDeleteButtons").data("no");
	var buttons = {};
	buttons[button_no] = function() {
	    $("#confirmDelete").dialog("close");
	};
	buttons[button_yes] = function() {
	    self.doDeletePlan(item, event);
	    $("#confirmDelete").dialog("close");
	};
	$("#confirmDelete").dialog({
	    modal : true,
	    height : 180,
	    width : 500,
	    buttons : buttons
	});
    };

    this.doDeletePlan = function(item, event) {
	$('#floorPlanTable').block({
	    message : '<div>Please wait ...</div>',
	    css : {
		border : '3px solid #a00'
	    }
	});
	var content = {};
	content.floorplan = item.uuid;
	content.uuid = agoController;
	content.command = 'deletefloorplan';
	sendCommand(content, function(res) {
	    if (res.result && res.result.returncode == 0) {
		self.floorplans.remove(function(e) {
		    return e.uuid == item.uuid;
		});
		delete localStorage.inventoryCache;
	    } else {
		alert("Error while deleting floorplan!");
	    }
	    $('#floorPlanTable').unblock();
	});
    };

    this.editPlan = function(item) {
	document.location.href = "/?floorplan&fp=" + item.uuid;
    };

}

/**
 * Initalizes the model
 */
function init_floorplanConfig() {
    model = new floorPlanConfig();

    model.mainTemplate = function() {
	return "configuration/floorplan";
    }.bind(model);

    model.navigation = function() {
	return "navigation/configuration";
    }.bind(model);

    ko.applyBindings(model);
}

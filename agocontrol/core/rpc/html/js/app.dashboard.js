/**
 * Model class
 * @returns {dashBoard}
 */
function dashBoard() {
    var self = this;
    this.devices = ko.observableArray([]);
    this.hasNavigation = ko.observable(false);
    this.keyword = ko.observable("");
    this.deviceList = ko.computed(function() {
	var list = self.devices();
	list = list.filter(function(dev) {
	    if (dev.devicetype == "event" || $.trim(dev.name) == "") {
		return false;
	    }
	    var keyword = self.keyword().toLowerCase();
	    if ($.trim(keyword) != "") {
		if (dev.devicetype.toLowerCase().indexOf(keyword) != -1) {
		    return true;
		}
		if (dev.name.toLowerCase().indexOf(keyword) != -1) {
		    return true;
		}
		if (dev.room.toLowerCase().indexOf(keyword) != -1) {
		    return true;
		}
		return false;
	    }

	    return true;
	});
	return list;
    });

    this.page = ko.observable(1);
    this.pages = ko.computed(function() {
	var pages = [];
	var max = Math.ceil(self.deviceList().length / 9);
	for ( var i = 1; i <= max; i++) {
	    pages.push({
		idx : i
	    });
	}
	return pages;
    });

    this.firstRow = ko.computed(function() {
	var currentList = self.deviceList().chunk(9);
	if (currentList.length < self.page()) {
	    return [];
	}
	currentList = currentList[self.page() - 1];
	if (currentList.length >= 0) {
	    return currentList.chunk(3)[0];
	}
	return [];
    });

    this.secondRow = ko.computed(function() {
	var currentList = self.deviceList().chunk(9);
	if (currentList.length < self.page()) {
	    return [];
	}
	currentList = currentList[self.page() - 1];
	if (currentList.length >= 4) {
	    return currentList.chunk(3)[1];
	}
	return [];
    });

    this.thirdRow = ko.computed(function() {
	var currentList = self.deviceList().chunk(9);
	if (currentList.length < self.page()) {
	    return [];
	}
	currentList = currentList[self.page() - 1];
	if (currentList.length >= 7) {
	    return currentList.chunk(3)[2];
	}
	return [];
    });

    this.switchPage = function(item) {
	self.page(item.idx);
    };

    buildfloorPlanList(this);
    buildPluginNamesList(this);
}

/**
 * Initalizes the model
 */
function init_dashBoard() {
    model = new dashBoard();

    model.deviceTemplate = function(item) {
	if (supported_devices.indexOf(item.devicetype) != -1) {
	    return 'devices/' + item.devicetype;
	}

	return 'devices/empty';
    }.bind(model);

    model.mainTemplate = function() {
	return "dashboard";
    }.bind(model);

    model.navigation = function() {
	return ""; // No navigation
    }.bind(model);

    ko.applyBindings(model);
}

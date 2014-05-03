Pebble.addEventListener("ready",
    function(e) {
    	// console.log("js ready");
    }
);


var forceUpdate = "false";

function sendLocation(latitude, longitude) {
	var response;
	var req = new XMLHttpRequest();
	req.open('POST', "https://vast-atoll-5515.herokuapp.com/watchupdate",true);
	req.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
	req.send("user=jsmith&latitude=" + latitude + "&longitude=" + longitude + "&force=" + forceUpdate);

	req.onload = function(e) {
		if (req.readyState == 4) {
			if (req.status == 200) {
				response = JSON.parse(req.responseText);
				console.log(response.note);
				if (response.update == 1) {
					// update with notes
					Pebble.showSimpleNotificationOnPebble(response.title,response.note);
				}
			}
		}
	};
}

function loadAddress(pos) {
	var coordinates = pos.coords;
	// request OSM reverse geocoder
	var response;
	var req = new XMLHttpRequest();
	req.open('GET', "http://open.mapquestapi.com/nominatim/v1/reverse.php?format=json&lat=" + coordinates.latitude + "&lon=" + coordinates.longitude, true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				response = JSON.parse(req.responseText);
				Pebble.sendAppMessage({
					1: response.address.road + "\n" + response.address.city
				});
			} else {
				console.log("Error");
			}
		}
		else {
			console.log("ready state error: " + req.readyState);
		}
	};
	req.send(null);

	sendLocation(coordinates.latitude, coordinates.longitude);
}

function locationError() {
	console.log("location error");
}


var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 


Pebble.addEventListener("appmessage", function(e) {
	// received message, send a post
	
	// console.log(e.type);
	// console.log("message!");
	if (e.payload.dummy == 10) {
		forceUpdate = "false";
	}
	else {
		forceUpdate = "true";
	}
	window.navigator.geolocation.getCurrentPosition(loadAddress, locationError, locationOptions);

});
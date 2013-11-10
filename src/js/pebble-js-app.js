
function getPrice() {
	var response;
	var req = new XMLHttpRequest();
	console.log("Sending...");
	// Token doesn't seem to be unique, as the API would suggest.
	// Consider migrating to direct call to BitcoinAverage API,
	// utilizing the config screen.
	console.log("Token: " + Pebble.getAccountToken());
	req.open('GET', "http://pebble.zyrenth.com/btc_price.json", true);
	req.setRequestHeader("X-Pebble-ID", Pebble.getAccountToken());
	req.onreadystatechange = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				console.log(req.responseText);
				response = JSON.parse(req.responseText);
				
				Pebble.sendAppMessage(response);
				
			} else {
				console.log("Error " + req.status);
			}
		}
	}
	req.send(null);
}

Pebble.addEventListener("ready", function(e) {
	console.log("connect!" + e.ready);
	console.log(e.type);
	getPrice();
});

Pebble.addEventListener("appmessage", function(e) {
	console.log(e.type);
	console.log(e.payload);
	console.log("message!");
	getPrice();
	

});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("webview closed");
	console.log(e.type);
	console.log(e.response);
});
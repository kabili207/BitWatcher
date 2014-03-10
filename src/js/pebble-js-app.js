var initialized = false;
var configuring = false;

function getOptions() {
	var options = JSON.parse(window.localStorage.getItem('options'));
	
	if(options == null) {
		options = { currency: "USD", exchange: "", average: true, noGox: true, invert: false };
		window.localStorage.setItem('options', JSON.stringify(options));
	}
	return options;
}

function getPrice() {
	var response;
	var req = new XMLHttpRequest();
	
	var options = getOptions();
	
	var url;
	var rates;
	
	if(options.average) {
		if(options.noGox)
			url = "https://api.bitcoinaverage.com/no-mtgox/ticker/" + options.currency;
		else
			url = "https://api.bitcoinaverage.com/ticker/" + options.currency;
	} else {
		url = "https://api.bitcoinaverage.com/exchanges/" + options.currency;
	}
	
	req.open('GET', url, true);
	req.onreadystatechange = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				rates = response = JSON.parse(req.responseText);
				if(!options.average)
					rates = response[options.exchange].rates;
				var message = {
					"currency": options.currency,
					"exchange" : options.average ? "average" : options.exchange,
					"ask": rates.ask.toString(),
					"bid": rates.bid.toString(),
					"last": rates.last.toString(),
					"invert": options.invert ? 1 : 0
				};
				if(!configuring){
					console.log("Sending...");
					console.log(JSON.stringify(message));
					Pebble.sendAppMessage(message);
				} else {
					console.log("Configuration open. Cancelling update.");
				}
			} else {
				console.log("Error " + req.status);
			}
		}
	}
	req.send(null);
}

Pebble.addEventListener("ready", function(e) {
	console.log("connect! " + e.ready);
	console.log(e.type);
	//getPrice();
});

Pebble.addEventListener("appmessage", function(e) {
	console.log("message! " + e.type);
	console.log(e.payload);
	getPrice();
});

Pebble.addEventListener("showConfiguration", function() {
	var options = getOptions();
	console.log("read options: " + JSON.stringify(options));
	console.log("showing configuration");
	var uri = 'http://kabili207.github.io/BitWatcher/config.html?config=' + encodeURIComponent(JSON.stringify(options));
	//var uri = 'http://zyrenth.com/~kabili/config.html?config=' + encodeURIComponent(JSON.stringify(options));
	configuring = true;
	Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("configuration closed");
	configuring = false;
	if (e.response != '') {
		var options = JSON.parse(decodeURIComponent(e.response));
		console.log("storing options: " + JSON.stringify(options));
		window.localStorage.setItem('options', JSON.stringify(options));
		getPrice();
	} else {
		console.log("no options received");
	}
});
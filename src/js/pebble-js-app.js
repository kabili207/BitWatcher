var initialized = false;

function getOptions() {
	var options = JSON.parse(window.localStorage.getItem('options'));
	
	if(options == null) {
		options = { currency: "USD", exchange: "", average: true, noGox: true };
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
					"exchange" : options.average ? (options.noGox ? " (no mtgox)" : "") : options.exchange,
					"ask": rates.ask.toString(),
					"bid": rates.bid.toString(),
					"last": rates.last.toString()
				};
				
				console.log("Sending...");
				console.log(JSON.stringify(message));
				Pebble.sendAppMessage(message);
				
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
	initialized = true;
	getPrice();
});

Pebble.addEventListener("appmessage", function(e) {
	console.log(e.type);
	console.log(e.payload);
	console.log("message!");
	getPrice();
	

});

Pebble.addEventListener("showConfiguration", function() {
	var options = getOptions();
	console.log("read options: " + JSON.stringify(options));
	console.log("showing configuration");
	var uri = 'http://kabili207.github.io/BitWatcher/config.html?config=' + encodeURIComponent(JSON.stringify(options));
	//var uri = 'http://zyrenth.com/~kabili/config.html?config=' + encodeURIComponent(JSON.stringify(options));
	Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
	console.log("configuration closed");
	if (e.response != '') {
		var options = JSON.parse(e.response);
		console.log("storing options: " + JSON.stringify(options));
		window.localStorage.setItem('options', JSON.stringify(options));
		getPrice();
	} else {
		console.log("no options received");
	}
});
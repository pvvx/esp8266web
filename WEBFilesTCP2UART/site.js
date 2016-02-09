/* Java for WEB device */
var ajaxList = new Array(); // Stores a queue of AJAX events to process
var nextimeout = 500;
function newAJAXCommand(url, container, repeat, data) {
	// Set up our object
	var newAjax = new Object();
	var theTimer = new Date();
	newAjax.url = url;
	newAjax.container = container;
	newAjax.repeat = repeat;
	newAjax.ajaxReq = null;
	// Create and send the request
	if (window.XMLHttpRequest) {
		newAjax.ajaxReq = new XMLHttpRequest();
		newAjax.ajaxReq.open((data == null) ? "GET" : "POST", newAjax.url, true);
		newAjax.ajaxReq.send(data);
		// If we're using IE6 style (maybe 5.5 compatible too)
	} else if (window.ActiveXObject) {
		newAjax.ajaxReq = new ActiveXObject("Microsoft.XMLHTTP");
		if (newAjax.ajaxReq) {
			newAjax.ajaxReq.open((data == null) ? "GET" : "POST", newAjax.url, true);
			newAjax.ajaxReq.send(data);
		}
	}
	newAjax.lastCalled = theTimer.getTime();
	// Store in our array
	ajaxList.push(newAjax);
}
function pollAJAX() {
	var curAjax = new Object();
	var theTimer = new Date();
	var elapsed;
	// Read off the ajaxList objects one by one
	for (i = ajaxList.length; i > 0; i--) {
		curAjax = ajaxList.shift();
		if (!curAjax)
			continue;
		elapsed = theTimer.getTime() - curAjax.lastCalled;
		// If we suceeded
		if (curAjax.ajaxReq.readyState == 4 && curAjax.ajaxReq.status == 200) {
			// If it has a container, write the result
			if (typeof (curAjax.container) == 'function')
				curAjax.container(curAjax.ajaxReq.responseXML.documentElement);
			else if (typeof (curAjax.container) == 'string')
				document.getElementById(curAjax.container).innerHTML = curAjax.ajaxReq.responseText;
			// (otherwise do nothing for null values)
			curAjax.ajaxReq.abort();
			curAjax.ajaxReq = null;
			// If it's a repeatable request, then do so
			if (curAjax.repeat) {
				if (elapsed >= curAjax.repeat)
					elapsed = 100;
				else
					elapsed = curAjax.repeat - elapsed;
				setTimeout("newAJAXCommand('" + curAjax.url + "',"
						+ curAjax.container + "," + curAjax.repeat + ")",
						elapsed);
			}
			continue;
		}
		// If we've waited over 4 second, then we timed out
		if ((curAjax.ajaxReq.readyState == 4 && curAjax.ajaxReq.status == 404)
				|| (elapsed > 4000)) {
			// Invoke the user function with null input
			if (typeof (curAjax.container) == 'function')
				curAjax.container(null);
			else
				// Alert the user
				alert("Command failed.\nConnection to device was lost.");
			curAjax.ajaxReq.abort();
			curAjax.ajaxReq = null;
			// If it's a repeatable request, then do so
			if (curAjax.repeat)
				setTimeout("newAJAXCommand('" + curAjax.url + "',"
						+ curAjax.container + "," + curAjax.repeat + ")", 200);
			continue;
		}
		// Otherwise, just keep waiting
		ajaxList.push(curAjax);
	}
	// Call ourselves again in 10ms?
	setTimeout("pollAJAX()", nextimeout);
}// End pollAjax
function getXMLValue(xmlData, field) {
	try {
		if (xmlData.getElementsByTagName(field)[0].firstChild.nodeValue)
			return xmlData.getElementsByTagName(field)[0].firstChild.nodeValue;
		else
			return null;
	} catch (err) {
		return null;
	}
}
//kick off the AJAX Updater
setTimeout("pollAJAX()", nextimeout);

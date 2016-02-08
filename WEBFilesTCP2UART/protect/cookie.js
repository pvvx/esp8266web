function getCookie(name) {
	var prefix = name + "=";
	var cookieStartIndex = document.cookie.indexOf(prefix);
	if (cookieStartIndex == -1)
		return null;
	var cookieEndIndex = document.cookie.indexOf(";", cookieStartIndex
			+ prefix.length);
	if (cookieEndIndex == -1)
		cookieEndIndex = document.cookie.length;
	return unescape(document.cookie.substring(cookieStartIndex + prefix.length,
			cookieEndIndex));
}
function setCookie(name, value) {
	document.cookie = name + "=" + escape(value) + "; path=/";
}
function setCookieElem(name, defv) {
	var val = getCookie(name);
	if (val == null || val.charAt(0) != '0' || val.charAt(1) != 'x') {
		val = defv;
		setCookie(name, val);
	}
	document.getElementById(name).value = val;
}
function NewCookie(add) {
	var val = parseInt(document.getElementById('start').value, 16) & 0xFFFFFFF0;
	if (val == NaN)
		setCookieElem('start', '0x40000000');
	else {
		val += add;
		setCookie('start', '0x' + val.toString(16));
		var nval = val + 256;
		setCookie('stop', '0x' + nval.toString(16));
		document.getElementById('start').value = '0x' + val.toString(16);
		document.getElementById('pmem').contentWindow.location.reload();
	}
}
setCookieElem('start', '0x40000000');
setCookieElem('set_ramaddr', '0x3FFF0000');
setCookieElem('set_ramdata', '0x12345678');
function UpTxt(xD, n, v) {
	var x = getXMLValue(xD, n, v);
	if (x == '?')
		document.getElementById("id_" + n).style.color = "#833";
	else
		document.getElementById("id_" + n).style.color = "#333";
	document.getElementById("id_" + n).innerHTML = x + v;
}
function UpdateValuesRam(xD) {
	if (xD) {
		UpTxt(xD, "ramaddr", "");
		UpTxt(xD, "ramdata", "");
	}
}
function SendRamVal(x) {
	var addr = parseInt(document.getElementById('set_ramaddr').value, 16);
	var val = parseInt(document.getElementById('set_ramdata').value, 16);
	if (addr != NaN && val != NaN) {
		document.getElementById('set_ramaddr').value = '0x' + addr.toString(16);
		setCookie('set_ramaddr','0x' + addr.toString(16));
		document.getElementById('set_ramdata').value = '0x' + val.toString(16);
		setCookie('set_ramdata','0x' + val.toString(16));
		if (x != 0)
			newAJAXCommand('chiprams.xml?start=0x' + addr.toString(16),
					UpdateValuesRam, 0);
		else
			newAJAXCommand('chiprams.xml?sys_ram0x' + addr.toString(16) + '=0x'
					+ val.toString(16) + '&start=0x' + addr.toString(16),
					UpdateValuesRam, 0);
	}
}
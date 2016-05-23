function slider(elemId, sliderWidth, range1, range2, step) {
	var knobWidth = 17;
	var knobHeight = 21;
	var sliderHeight = 21;
	var offsX, tmp;
	var d = document;
	var isIE = d.all || window.opera;
	var point = (sliderWidth - knobWidth - 3) / (range2 - range1);
	var slider = d.createElement('DIV');
	slider.id = elemId + '_slider';
	slider.className = 'slider';
	d.getElementById(elemId).appendChild(slider);
	var knob = d.createElement('DIV');
	knob.id = elemId + '_knob';
	knob.className = 'knob';
	slider.appendChild(knob);
	knob.style.left = 0;
	knob.style.width = knobWidth + 'px';
	knob.style.height = knobHeight + 'px';
	slider.style.width = sliderWidth + 'px';
	slider.style.height = sliderHeight + 'px';
	var sliderOffset = slider.offsetLeft;
	tmp = slider.offsetParent;
	while (tmp.tagName != 'BODY') {
		sliderOffset += tmp.offsetLeft;
		tmp = tmp.offsetParent
	}
	if (isIE) {
		knob.onmousedown = startCoord;
		slider.onclick = sliderClick;
		knob.onmouseup = endCoord;
		slider.onmouseup = endCoord
	} else {
		knob.addEventListener("mousedown", startCoord, true);
		slider.addEventListener("click", sliderClick, true);
		knob.addEventListener("mouseup", endCoord, true);
		slider.addEventListener("mouseup", endCoord, true)
	}
	function setValue(x) {
		if (x < 0)
			knob.style.left = 0;
		else if (x > sliderWidth - knobWidth - 3)
			knob.style.left = (sliderWidth - 3 - knobWidth) + 'px';
		else {
			if (step == 0)
				knob.style.left = x + 'px';
			else
				knob.style.left = Math.round(x / (step * point)) * step * point
						+ 'px'
		}
		nextimeout = getValue();
		d.getElementById('toutid').value = nextimeout;
		document.getElementById('toutid').innerHTML = nextimeout
	}
	function setValue2(x) {
		if (x < range1 || x > range2)
			alert('Value is not included into a slider range!');
		else
			setValue((x - range1) * point);
		nextimeout = getValue();
		d.getElementById('toutid').value = nextimeout;
		document.getElementById('toutid').innerHTML = nextimeout
	}
	function getValue() {
		return Math.round(parseInt(knob.style.left) / point) + range1
	}
	function sliderClick(e) {
		var x;
		if (isIE) {
			if (event.srcElement != slider)
				return;
			x = event.offsetX - Math.round(knobWidth / 2)
		} else
			x = e.pageX - sliderOffset - knobWidth / 2;
		setValue(x)
	}
	function startCoord(e) {
		if (isIE) {
			offsX = event.clientX - parseInt(knob.style.left);
			slider.onmousemove = mov
		} else {
			slider.addEventListener("mousemove", mov, true)
		}
	}
	function mov(e) {
		var x;
		if (isIE)
			x = event.clientX - offsX;
		else
			x = e.pageX - sliderOffset - knobWidth / 2;
		setValue(x)
	}
	function endCoord() {
		if (isIE)
			slider.onmousemove = null;
		else
			slider.removeEventListener("mousemove", mov, true)
	}
	this.setValue = setValue2;
	this.getValue = getValue
}

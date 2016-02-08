;
(function(exports) {
	var Util = {
		extend : function() {
			arguments[0] = arguments[0] || {};
			for (var i = 1; i < arguments.length; i++) {
				for ( var key in arguments[i]) {
					if (arguments[i].hasOwnProperty(key)) {
						if (typeof (arguments[i][key]) === 'object') {
							if (arguments[i][key] instanceof Array) {
								arguments[0][key] = arguments[i][key]
							} else {
								arguments[0][key] = Util.extend(
										arguments[0][key], arguments[i][key])
							}
						} else {
							arguments[0][key] = arguments[i][key]
						}
					}
				}
			}
			return arguments[0]
		}
	};
	function TimeSeries(options) {
		this.options = Util.extend({}, TimeSeries.defaultOptions, options);
		this.clear()
	}
	TimeSeries.defaultOptions = {
		resetBoundsInterval : 3000,
		resetBounds : true
	};
	TimeSeries.prototype.clear = function() {
		this.data = [];
		this.maxValue = Number.NaN;
		this.minValue = Number.NaN
	};
	TimeSeries.prototype.resetBounds = function() {
		if (this.data.length) {
			this.maxValue = this.data[0][1];
			this.minValue = this.data[0][1];
			for (var i = 1; i < this.data.length; i++) {
				var value = this.data[i][1];
				if (value > this.maxValue) {
					this.maxValue = value
				}
				if (value < this.minValue) {
					this.minValue = value
				}
			}
		} else {
			this.maxValue = Number.NaN;
			this.minValue = Number.NaN
		}
	};
	TimeSeries.prototype.append = function(timestamp, value,
			sumRepeatedTimeStampValues) {
		var i = this.data.length - 1;
		while (i >= 0 && this.data[i][0] > timestamp) {
			i--
		}
		if (i === -1) {
			this.data.splice(0, 0, [ timestamp, value ])
		} else if (this.data.length > 0 && this.data[i][0] === timestamp) {
			if (sumRepeatedTimeStampValues) {
				this.data[i][1] += value;
				value = this.data[i][1]
			} else {
				this.data[i][1] = value
			}
		} else if (i < this.data.length - 1) {
			this.data.splice(i + 1, 0, [ timestamp, value ])
		} else {
			this.data.push([ timestamp, value ])
		}
		this.maxValue = isNaN(this.maxValue) ? value : Math.max(this.maxValue,
				value);
		this.minValue = isNaN(this.minValue) ? value : Math.min(this.minValue,
				value)
	};
	TimeSeries.prototype.dropOldData = function(oldestValidTime,
			maxDataSetLength) {
		var removeCount = 0;
		while (this.data.length - removeCount >= maxDataSetLength
				&& this.data[removeCount + 1][0] < oldestValidTime) {
			removeCount++
		}
		if (removeCount !== 0) {
			this.data.splice(0, removeCount)
		}
	};
	function SmoothieChart(options) {
		this.options = Util.extend({}, SmoothieChart.defaultChartOptions,
				options);
		this.seriesSet = [];
		this.currentValueRange = 1;
		this.currentVisMinValue = 0;
		this.lastRenderTimeMillis = 0
	}
	SmoothieChart.defaultChartOptions = {
		millisPerPixel : 20,
		enableDpiScaling : true,
		yMinFormatter : function(min, precision) {
			return parseFloat(min).toFixed(precision)
		},
		yMaxFormatter : function(max, precision) {
			return parseFloat(max).toFixed(precision)
		},
		maxValueScale : 1,
		interpolation : 'bezier',
		scaleSmoothing : 0.125,
		maxDataSetLength : 2,
		grid : {
			fillStyle : '#000000',
			strokeStyle : '#777777',
			lineWidth : 1,
			sharpLines : false,
			millisPerLine : 1000,
			verticalSections : 2,
			borderVisible : true
		},
		labels : {
			fillStyle : '#ffffff',
			disabled : false,
			fontSize : 10,
			fontFamily : 'monospace',
			precision : 2
		},
		horizontalLines : []
	};
	SmoothieChart.AnimateCompatibility = (function() {
		var requestAnimationFrame = function(callback, element) {
			var requestAnimationFrame = window.requestAnimationFrame
					|| window.webkitRequestAnimationFrame
					|| window.mozRequestAnimationFrame
					|| window.oRequestAnimationFrame
					|| window.msRequestAnimationFrame || function(callback) {
						return window.setTimeout(function() {
							callback(new Date().getTime())
						}, 16)
					};
			return requestAnimationFrame.call(window, callback, element)
		}, cancelAnimationFrame = function(id) {
			var cancelAnimationFrame = window.cancelAnimationFrame
					|| function(id) {
						clearTimeout(id)
					};
			return cancelAnimationFrame.call(window, id)
		};
		return {
			requestAnimationFrame : requestAnimationFrame,
			cancelAnimationFrame : cancelAnimationFrame
		}
	})();
	SmoothieChart.defaultSeriesPresentationOptions = {
		lineWidth : 1,
		strokeStyle : '#ffffff'
	};
	SmoothieChart.prototype.addTimeSeries = function(timeSeries, options) {
		this.seriesSet.push({
			timeSeries : timeSeries,
			options : Util.extend({},
					SmoothieChart.defaultSeriesPresentationOptions, options)
		});
		if (timeSeries.options.resetBounds
				&& timeSeries.options.resetBoundsInterval > 0) {
			timeSeries.resetBoundsTimerId = setInterval(function() {
				timeSeries.resetBounds()
			}, timeSeries.options.resetBoundsInterval)
		}
	};
	SmoothieChart.prototype.removeTimeSeries = function(timeSeries) {
		var numSeries = this.seriesSet.length;
		for (var i = 0; i < numSeries; i++) {
			if (this.seriesSet[i].timeSeries === timeSeries) {
				this.seriesSet.splice(i, 1);
				break
			}
		}
		if (timeSeries.resetBoundsTimerId) {
			clearInterval(timeSeries.resetBoundsTimerId)
		}
	};
	SmoothieChart.prototype.getTimeSeriesOptions = function(timeSeries) {
		var numSeries = this.seriesSet.length;
		for (var i = 0; i < numSeries; i++) {
			if (this.seriesSet[i].timeSeries === timeSeries) {
				return this.seriesSet[i].options
			}
		}
	};
	SmoothieChart.prototype.bringToFront = function(timeSeries) {
		var numSeries = this.seriesSet.length;
		for (var i = 0; i < numSeries; i++) {
			if (this.seriesSet[i].timeSeries === timeSeries) {
				var set = this.seriesSet.splice(i, 1);
				this.seriesSet.push(set[0]);
				break
			}
		}
	};
	SmoothieChart.prototype.streamTo = function(canvas, delayMillis) {
		this.canvas = canvas;
		this.delay = delayMillis;
		this.start()
	};
	SmoothieChart.prototype.start = function() {
		if (this.frame) {
			return
		}
		if (this.options.enableDpiScaling && window
				&& window.devicePixelRatio !== 1) {
			var canvasWidth = this.canvas.getAttribute('width');
			var canvasHeight = this.canvas.getAttribute('height');
			this.canvas.setAttribute('width', canvasWidth
					* window.devicePixelRatio);
			this.canvas.setAttribute('height', canvasHeight
					* window.devicePixelRatio);
			this.canvas.style.width = canvasWidth + 'px';
			this.canvas.style.height = canvasHeight + 'px';
			this.canvas.getContext('2d').scale(window.devicePixelRatio,
					window.devicePixelRatio)
		}
		var animate = function() {
			this.frame = SmoothieChart.AnimateCompatibility
					.requestAnimationFrame(function() {
						this.render();
						animate()
					}.bind(this))
		}.bind(this);
		animate()
	};
	SmoothieChart.prototype.stop = function() {
		if (this.frame) {
			SmoothieChart.AnimateCompatibility.cancelAnimationFrame(this.frame);
			delete this.frame
		}
	};
	SmoothieChart.prototype.updateValueRange = function() {
		var chartOptions = this.options, chartMaxValue = Number.NaN, chartMinValue = Number.NaN;
		for (var d = 0; d < this.seriesSet.length; d++) {
			var timeSeries = this.seriesSet[d].timeSeries;
			if (!isNaN(timeSeries.maxValue)) {
				chartMaxValue = !isNaN(chartMaxValue) ? Math.max(chartMaxValue,
						timeSeries.maxValue) : timeSeries.maxValue
			}
			if (!isNaN(timeSeries.minValue)) {
				chartMinValue = !isNaN(chartMinValue) ? Math.min(chartMinValue,
						timeSeries.minValue) : timeSeries.minValue
			}
		}
		if (chartOptions.maxValue != null) {
			chartMaxValue = chartOptions.maxValue
		} else {
			chartMaxValue *= chartOptions.maxValueScale
		}
		if (chartOptions.minValue != null) {
			chartMinValue = chartOptions.minValue
		}
		if (this.options.yRangeFunction) {
			var range = this.options.yRangeFunction({
				min : chartMinValue,
				max : chartMaxValue
			});
			chartMinValue = range.min;
			chartMaxValue = range.max
		}
		if (!isNaN(chartMaxValue) && !isNaN(chartMinValue)) {
			var targetValueRange = chartMaxValue - chartMinValue;
			var valueRangeDiff = (targetValueRange - this.currentValueRange);
			var minValueDiff = (chartMinValue - this.currentVisMinValue);
			this.isAnimatingScale = Math.abs(valueRangeDiff) > 0.1
					|| Math.abs(minValueDiff) > 0.1;
			this.currentValueRange += chartOptions.scaleSmoothing
					* valueRangeDiff;
			this.currentVisMinValue += chartOptions.scaleSmoothing
					* minValueDiff
		}
		this.valueRange = {
			min : chartMinValue,
			max : chartMaxValue
		}
	};
	SmoothieChart.prototype.render = function(canvas, time) {
		var nowMillis = new Date().getTime();
		if (!this.isAnimatingScale) {
			var maxIdleMillis = Math.min(1000 / 6, this.options.millisPerPixel);
			if (nowMillis - this.lastRenderTimeMillis < maxIdleMillis) {
				return
			}
		}
		this.lastRenderTimeMillis = nowMillis;
		canvas = canvas || this.canvas;
		time = time || nowMillis - (this.delay || 0);
		time -= time % this.options.millisPerPixel;
		var context = canvas.getContext('2d'), chartOptions = this.options, dimensions = {
			top : 0,
			left : 0,
			width : canvas.clientWidth,
			height : canvas.clientHeight
		}, oldestValidTime = time
				- (dimensions.width * chartOptions.millisPerPixel), valueToYPixel = function(
				value) {
			var offset = value - this.currentVisMinValue;
			return this.currentValueRange === 0 ? dimensions.height
					: dimensions.height
							- (Math.round((offset / this.currentValueRange)
									* dimensions.height))
		}.bind(this), timeToXPixel = function(t) {
			return Math.round(dimensions.width
					- ((time - t) / chartOptions.millisPerPixel))
		};
		this.updateValueRange();
		context.font = chartOptions.labels.fontSize + 'px '
				+ chartOptions.labels.fontFamily;
		context.save();
		context.translate(dimensions.left, dimensions.top);
		context.beginPath();
		context.rect(0, 0, dimensions.width, dimensions.height);
		context.clip();
		context.save();
		context.fillStyle = chartOptions.grid.fillStyle;
		context.clearRect(0, 0, dimensions.width, dimensions.height);
		context.fillRect(0, 0, dimensions.width, dimensions.height);
		context.restore();
		context.save();
		context.lineWidth = chartOptions.grid.lineWidth;
		context.strokeStyle = chartOptions.grid.strokeStyle;
		if (chartOptions.grid.millisPerLine > 0) {
			context.beginPath();
			for (var t = time - (time % chartOptions.grid.millisPerLine); t >= oldestValidTime; t -= chartOptions.grid.millisPerLine) {
				var gx = timeToXPixel(t);
				if (chartOptions.grid.sharpLines) {
					gx -= 0.5
				}
				context.moveTo(gx, 0);
				context.lineTo(gx, dimensions.height)
			}
			context.stroke();
			context.closePath()
		}
		for (var v = 1; v < chartOptions.grid.verticalSections; v++) {
			var gy = Math.round(v * dimensions.height
					/ chartOptions.grid.verticalSections);
			if (chartOptions.grid.sharpLines) {
				gy -= 0.5
			}
			context.beginPath();
			context.moveTo(0, gy);
			context.lineTo(dimensions.width, gy);
			context.stroke();
			context.closePath()
		}
		if (chartOptions.grid.borderVisible) {
			context.beginPath();
			context.strokeRect(0, 0, dimensions.width, dimensions.height);
			context.closePath()
		}
		context.restore();
		if (chartOptions.horizontalLines && chartOptions.horizontalLines.length) {
			for (var hl = 0; hl < chartOptions.horizontalLines.length; hl++) {
				var line = chartOptions.horizontalLines[hl], hly = Math
						.round(valueToYPixel(line.value)) - 0.5;
				context.strokeStyle = line.color || '#ffffff';
				context.lineWidth = line.lineWidth || 1;
				context.beginPath();
				context.moveTo(0, hly);
				context.lineTo(dimensions.width, hly);
				context.stroke();
				context.closePath()
			}
		}
		for (var d = 0; d < this.seriesSet.length; d++) {
			context.save();
			var timeSeries = this.seriesSet[d].timeSeries, dataSet = timeSeries.data, seriesOptions = this.seriesSet[d].options;
			timeSeries.dropOldData(oldestValidTime,
					chartOptions.maxDataSetLength);
			context.lineWidth = seriesOptions.lineWidth;
			context.strokeStyle = seriesOptions.strokeStyle;
			context.beginPath();
			var firstX = 0, lastX = 0, lastY = 0;
			for (var i = 0; i < dataSet.length && dataSet.length !== 1; i++) {
				var x = timeToXPixel(dataSet[i][0]), y = valueToYPixel(dataSet[i][1]);
				if (i === 0) {
					firstX = x;
					context.moveTo(x, y)
				} else {
					switch (chartOptions.interpolation) {
					case "linear":
					case "line": {
						context.lineTo(x, y);
						break
					}
					case "bezier":
					default: {
						context.bezierCurveTo(Math.round((lastX + x) / 2),
								lastY, Math.round((lastX + x)) / 2, y, x, y);
						break
					}
					case "step": {
						context.lineTo(x, lastY);
						context.lineTo(x, y);
						break
					}
					}
				}
				lastX = x;
				lastY = y
			}
			if (dataSet.length > 1) {
				if (seriesOptions.fillStyle) {
					context.lineTo(dimensions.width + seriesOptions.lineWidth
							+ 1, lastY);
					context.lineTo(dimensions.width + seriesOptions.lineWidth
							+ 1, dimensions.height + seriesOptions.lineWidth
							+ 1);
					context.lineTo(firstX, dimensions.height
							+ seriesOptions.lineWidth);
					context.fillStyle = seriesOptions.fillStyle;
					context.fill()
				}
				if (seriesOptions.strokeStyle
						&& seriesOptions.strokeStyle !== 'none') {
					context.stroke()
				}
				context.closePath()
			}
			context.restore()
		}
		if (!chartOptions.labels.disabled && !isNaN(this.valueRange.min)
				&& !isNaN(this.valueRange.max)) {
			var maxValueString = chartOptions.yMaxFormatter(
					this.valueRange.max, chartOptions.labels.precision), minValueString = chartOptions
					.yMinFormatter(this.valueRange.min,
							chartOptions.labels.precision);
			context.fillStyle = chartOptions.labels.fillStyle;
			context.fillText(maxValueString, dimensions.width
					- context.measureText(maxValueString).width - 2,
					chartOptions.labels.fontSize);
			context.fillText(minValueString, dimensions.width
					- context.measureText(minValueString).width - 2,
					dimensions.height - 2)
		}
		if (chartOptions.timestampFormatter
				&& chartOptions.grid.millisPerLine > 0) {
			var textUntilX = dimensions.width
					- context.measureText(minValueString).width + 4;
			for (var t = time - (time % chartOptions.grid.millisPerLine); t >= oldestValidTime; t -= chartOptions.grid.millisPerLine) {
				var gx = timeToXPixel(t);
				if (gx < textUntilX) {
					var tx = new Date(t), ts = chartOptions
							.timestampFormatter(tx), tsWidth = context
							.measureText(ts).width;
					textUntilX = gx - tsWidth - 2;
					context.fillStyle = chartOptions.labels.fillStyle;
					context.fillText(ts, gx - tsWidth, dimensions.height - 2)
				}
			}
		}
		context.restore()
	};
	SmoothieChart.timeFormatter = function(date) {
		function pad2(number) {
			return (number < 10 ? '0' : '') + number
		}
		return pad2(date.getHours()) + ':' + pad2(date.getMinutes()) + ':'
				+ pad2(date.getSeconds())
	};
	exports.TimeSeries = TimeSeries;
	exports.SmoothieChart = SmoothieChart
})(typeof exports === 'undefined' ? this : exports);
var line1 = new TimeSeries();
var newval = 0;
function addpoint(xmlData) {
	if (xmlData) {
		newval = eval(getXMLValue(xmlData, 'value'));
		line1.append(new Date().getTime(), newval);
		document.getElementById('xdata').innerHTML = newval;
		if (newval > xmax)
			document.getElementById('xdata').style.color = '#0000A0';
		else if (newval < xmin)
			document.getElementById('xdata').style.color = '#A00000';
		else
			document.getElementById('xdata').style.color = '#00A000'
	} else
		line1.append(new Date().getTime(), newval)
}
var smoothie = new SmoothieChart({
	interpolation : 'linear',
	minValue : 0,
	millisPerPixel : millisPerPixel,
	grid : {
		strokeStyle : 'rgb(100, 110, 150)',
		fillStyle : 'rgb(50, 55, 75)',
		lineWidth : 1,
		millisPerLine : millisPerLine,
		verticalSections : 6
	},
	labels : {
		precision : 0
	}
});
smoothie.addTimeSeries(line1, {
	strokeStyle : 'rgb(255, 0, 200)',
	fillStyle : 'rgba(255, 0, 200, 0.3)',
	lineWidth : 3
});
smoothie.streamTo(document.getElementById("mycanvas"), nextimeout);
setTimeout("newAJAXCommand(xmlfile, addpoint, true)", 100);
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
var mysl1 = new slider('sl', 333, 20, 10020, 0);
mysl1.setValue(500);
document.getElementById('toutid').innerHTML = mysl1.getValue();
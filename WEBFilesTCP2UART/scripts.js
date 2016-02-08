var setFormValues = function(form, cfg) {
	var name, field;
	for (name in cfg){
		if (form[name]) {
			field = form[name];
			if (field[1] && field[1].type === 'checkbox') {
				field = field[1];
			}
			if (field.type === 'checkbox'){
				field.checked = cfg[name] === '1' ? true : false;
			} else {
				field.value = cfg[name];
			}
		}
	}
}
var $ = function(id) {
	return document.getElementById(id);
}
var reloadTimer={
	s:10,
	reload:function(start) {
		if(start) {
			this.s = start;
		}
		$('timer').innerHTML = this.s < 10 ? '0' + this.s : this.s;
		if (this.s == 0){
			document.location.href = document.referrer != '' ? document.referrer : '/';
		}
		this.s--;
		setTimeout('reloadTimer.reload()', 1000);
	}
}

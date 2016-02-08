<script type="text/javascript">
var coef = ~sys_ucnst_0~.0;
if(coef == Infinity || coef > 1000000.0 || coef <= 100.0) coef = 76500.0;
document.getElementById('syspower').innerHTML = eval(~sys_vdd33~*100.0/coef).toFixed(2);
</script>
<script type="text/javascript">
var x = ~sntp_time~*1000;
if(x){
var d = new Date(x);
document.getElementById('sntptime').innerHTML= d.toLocaleDateString()+" "+d.toLocaleTimeString();
}
</script>
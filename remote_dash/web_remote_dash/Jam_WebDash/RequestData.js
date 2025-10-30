// Domain values:

domain = 
'baja.403587.xyz' //Colin's server (Test data source (Soon outdated))
//'192.168.0.67' //Rasberry pi (Test middleman)
//'x' //Brock's server (Competition middleman)
;


//URL for access
const baseURL = 'http://' + domain + '/FakeData/battery_voltage/';


//Function to call:
function pull_data(){

        $.ajax({
            method: 'GET', 
            url: "http://127.0.0.1:5000/helloworld", 
            cache: false, 
            data: JSON})
        .done(function(data){
            console.log(data);
        })

}

// Domain values:

domain = 
'baja.403587.xyz' //Colin's server (Test data source (Soon outdated))
//'192.168.0.67' //Rasberry pi (Test middleman)
//'x' //Brock's server (Competition middleman)
;


//URL for access
//const baseURL = 'http://' + domain + '/FakeData/battery_voltage/';


//Function to call:
function pull_data(){

        $.ajax({
            method: 'GET', 
            url: "http:/baja.403587.xyz/data/?since=5%20minutes", 
            cache: false, 
            data: JSON})
        .done(function(data){
            console.log(data.Logging_Event);
            dataArray_str = data.Logging_Data;
            console.log(dataArray_str);
            for(k in dataArray_str){
                current_data = dataArray_str[k];
                console.log(current_data.type);
                const datatimestamp = new Date(current_data.timestamp)
                switch (current_data.type){
                    case "Voltage":
                        document.getElementById("battery_voltage_data").innerHTML = current_data.data + " Volts at " + datatimestamp;
                    break;
                }
            }

        })

}
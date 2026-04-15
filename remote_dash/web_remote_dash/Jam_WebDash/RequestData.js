
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
        TargetDataFile = "";
        RequestStartTime = Date.now();
        $.ajax({
            method: 'GET', 
            url: "https://baja.403587.xyz/logs/json_received/list_files", 
            cache: false, 
            data: JSON})
        .done(function(data){
            console.log(data);
            console.log(data.files[0].name);
            TargetDataFile = data.files[0].name;
            $.ajax({
                method: 'GET', 
                url: "https://baja.403587.xyz/logs/json_received/open?name=" + TargetDataFile, 
                cache: false, 
                data: JSON})
            .done(function(data){
                console.log(data.received_json.Logging_Data);
                dataArray = data.received_json.Logging_Data;

                ping = Date.now() -  RequestStartTime;
                console.log(ping);
                document.getElementById("ServerInfo").innerHTML = "Server ping:  "+ ping + "ms  | Car status: Disconnected";

                for(k in dataArray){
                    current_data = dataArray[k];
                    
                    const datatimestamp = current_data.timestamp;
                    switch (current_data.type){
                        case "temp":
                            document.getElementById("TRANSTEMP").innerHTML = current_data.data[0] + " C | " + datatimestamp;
                        break;
                        case "gps":
                            myMarkerPlayer.moveTo({latlng: [current_data.data[0], current_data.data[1]]}, ping);
                    }
                    
                }
            })
        })


}








/*
            for(k in dataArray_str){
                current_data = dataArray_str[k];
                console.log(current_data.type);
                const datatimestamp = new Date(current_data.timestamp)
                switch (current_data.type){
                    case "Voltage":
                        document.getElementById("battery_voltage_data").innerHTML = current_data.data + " Volts at " + datatimestamp;
                    break;
                }
                    
            }*/
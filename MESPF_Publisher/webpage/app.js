/**
 * Add gobals here
 */
var seconds 	= null;
var otaTimerVar =  null;

/**
 * Initialize functions here.
 */
$(document).ready(function(){
	getUpdateStatus();
	getConnectionsConfiguration();
});

function getConnectionsConfiguration()
{
    var xhr = new XMLHttpRequest();
    var requestURL = "/ConnectionsConfiguration";		//ConnectionsConfiguration has to be handled
    xhr.open('POST', requestURL, false);
    xhr.send();
    
     if (xhr.readyState == 4 && xhr.status == 200) {
     	var response = JSON.parse(xhr.responseText);
     	
		document.getElementById("WIFI_SSID").value = response.WIFI_SSID;
		document.getElementById("WIFI_PASS").value = response.WIFI_PASS;
		if (response.WIFI_STATUS == 1) document.getElementById("WIFI_STATUS").style.color = 'green';
		else if (response.WIFI_STATUS == 0) document.getElementById("WIFI_STATUS").style.color = 'red';
		else document.getElementById("WIFI_STATUS").style.color = 'yellow';
		
		document.getElementById("MQTT_IP").value = response.MQTT_IP;
		document.getElementById("MQTT_USER").value = response.MQTT_USER;
		document.getElementById("MQTT_PASS").value = response.MQTT_PASS;
		if (response.MQTT_STATUS == 1) document.getElementById("MQTT_STATUS").style.color = 'green';
		else if (response.MQTT_STATUS == 0) document.getElementById("MQTT_STATUS").style.color = 'red';
		else document.getElementById("MQTT_STATUS").style.color = 'yellow';
	}
}
   

/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() 
{
    var x = document.getElementById("selected_file");
    var file = x.files[0];

    document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
}

/**
 * Handles the firmware update.
 */
function updateFirmware() 
{
    // Form Data
    var formData = new FormData();
    var fileSelect = document.getElementById("selected_file");
    
    if (fileSelect.files && fileSelect.files.length == 1) 
	{
        var file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Http Request
        var request = new XMLHttpRequest();

        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");		// OTAupdate has to be handled
        request.responseType = "blob";
        request.send(formData);
    } 
	else 
	{
        window.alert('Select A File First')
    }
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) 
{
    if (oEvent.lengthComputable) 
	{
        getUpdateStatus();
    } 
	else 
	{
        window.alert('total size is unknown')
    }
}

/**
 * Posts the firmware udpate status.
 */
function getUpdateStatus() 
{
    var xhr = new XMLHttpRequest();
    var requestURL = "/OTAstatus";		//OTAstatus has to be handled
    xhr.open('POST', requestURL, false);
    xhr.send('ota_update_status');

    if (xhr.readyState == 4 && xhr.status == 200) 
	{		
        var response = JSON.parse(xhr.responseText);
						
	 	document.getElementById("latest_firmware").innerHTML = response.compile_date + " - " + response.compile_time

		// If flashing was complete it will return a 1, else -1
		// A return of 0 is just for information on the Latest Firmware request
        if (response.ota_update_status == 1) 
		{
    		// Set the countdown timer time
            seconds = 10;
            // Start the countdown timer
            otaRebootTimer();
        } 
        else if (response.ota_update_status == -1)
		{
            document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
        }
    }
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() 
{	
    document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds == 0) 
	{
        clearTimeout(otaTimerVar);
        window.location.reload();
    } 
	else 
	{
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}

function submitWifiConf()
{
	window.alert("SSID: " + document.getElementById('WIFI_SSID').value + "\n" + "PASSWORD: " + document.getElementById('WIFI_PASS').value);
	document.getElementById('WIFI_SSID').disabled = true;
	document.getElementById('WIFI_PASS').disabled = true;
	document.getElementById('WIFI_EDIT').hidden = false;
	document.getElementById('WIFI_SUBMIT').hidden = true;
	getConnectionsConfiguration();
}

function editWifiConf()
{
	document.getElementById('WIFI_SSID').disabled = false;
	document.getElementById('WIFI_PASS').disabled = false;
	document.getElementById('WIFI_EDIT').hidden = true;
	document.getElementById('WIFI_SUBMIT').hidden = false;
	getConnectionsConfiguration();
}

function submitMQTTConf()
{
	window.alert("IP: " + document.getElementById('MQTT_IP').value + "\n" + "USER: " + document.getElementById('MQTT_USER').value +"\n" +"PASSWORD: " + document.getElementById('MQTT_PASS').value);
	document.getElementById('MQTT_IP').disabled = true;
	document.getElementById('MQTT_USER').disabled = true;
	document.getElementById('MQTT_PASS').disabled = true;
	document.getElementById('MQTT_EDIT').hidden = false;
	document.getElementById('MQTT_SUBMIT').hidden = true;
	getConnectionsConfiguration();
}

function editMQTTConf()
{
	document.getElementById('MQTT_IP').disabled = false;
	document.getElementById('MQTT_USER').disabled = false;
	document.getElementById('MQTT_PASS').disabled = false;
	document.getElementById('MQTT_EDIT').hidden = true;
	document.getElementById('MQTT_SUBMIT').hidden = false;
	getConnectionsConfiguration();
}



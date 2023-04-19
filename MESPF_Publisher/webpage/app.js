/**
 * Add gobals here
 */
var tab				= 0;
var seconds 		= null;
var otaTimerVar 	= null;

// Info related to the sensors 
var sensorsInstalled = null;
const gpioOptions = ["None"];

/**
 * Initialize functions here.
 */
$(document).ready(function(){
	
	getUpdateStatus();
	getConnectionsConfiguration();
	
	initSensors();
	
	setTimeout(createAddButtons, 1000);
	setTimeout(startSensorsValuesUpdateInterval, 1000);
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
		if (response.WIFI_STATUS == 1) document.getElementById("WIFI_STATUS").style.backgroundColor = 'green';
		else if (response.WIFI_STATUS == 0) document.getElementById("WIFI_STATUS").style.backgroundColor = 'orange';
		else document.getElementById("WIFI_STATUS").style.backgroundColor = 'red';
		
		document.getElementById("NTP_SERVER").value = response.NTP_SERVER;
		document.getElementById("NTP_SYNC").value = response.NTP_SYNC/1000;
		if (response.NTP_STATUS == 1) document.getElementById("NTP_STATUS").style.backgroundColor = 'green';
		else if (response.NTP_STATUS == 0) document.getElementById("NTP_STATUS").style.backgroundColor = 'orange';
		else document.getElementById("NTP_STATUS").style.backgroundColor = 'red';		
		
		document.getElementById("MQTT_IP").value = response.MQTT_IP;
		document.getElementById("MQTT_PORT").value = response.MQTT_PORT;
		document.getElementById("MQTT_USER").value = response.MQTT_USER;
		document.getElementById("MQTT_PASS").value = response.MQTT_PASS;
		if (response.MQTT_STATUS == 1) document.getElementById("MQTT_STATUS").style.backgroundColor = 'green';
		else if (response.MQTT_STATUS == 0) document.getElementById("MQTT_STATUS").style.backgroundColor = 'orange';
		else document.getElementById("MQTT_STATUS").style.backgroundColor = 'red';
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

/**
 * Clear HTML div
 */
function clearDiv(divID){
	document.getElementById(divID).innerHTML = "";
}

/**
 * Remove HTML div 
 */
function removeDiv(divID){
	document.getElementById(divID).remove();
}

/**
 * Initializes sensors
 */
function initSensors(){
	
	// Get sensors installed
	getSensorsConfiguration();
			
	// Fill the information about them
	setTimeout(sensorsValuesUpdate, 1000);
	setTimeout(sensorsGpiosUpdate, 1000);
	setTimeout(sensorsParametersUpdate, 1000);
	setTimeout(sensorsLocationsUpdate, 1000);
	setTimeout(sensorsAlertsUpdate, 1000);
	
	setTimeout(gpiosAvailablesUpdate, 1000);
	setTimeout(fillAndUpdateOptions, 1500);
}

/**
 * Create all necessary buttons to install the different types of sensors
 */
function createAddButtons(){
	
	for(let i = 1; i < sensorsInstalled; i++){
		if(document.getElementById("sensor_" + i).dataset.sensor_name != "null")
			document.getElementById("add_sensors_buttons").innerHTML += '<input type="button" value="Add '+ document.getElementById("sensor_" + i).dataset.sensor_name +'" onclick="deploySensorForm('+ i +')" />';
	}
}

/**
 * Get the Sensors already installed
 */
function getSensorsConfiguration(){
	
    $.getJSON('/SensorsConfiguration', function(data){
    	
    	var name, nUnits, nValues, valuesNames, valuesTypes, nGpios, gpiosNames, auxGpiosNames, nParameters, parametersNames, auxParametersNames, parametersTypes, auxParametersTypes;
    	
    	sensorsInstalled = data["nSensors"];
    	
    	for(let i = 1; i < sensorsInstalled; i++){
    		
    		name = data["sensors"][i].sensorName;
    		nUnits = data["sensors"][i].numberOfUnits;
    		nValues = data["sensors"][i].numberOfValues;
    		valuesNames = data["sensors"][i].valueNames;
    		valuesTypes = data["sensors"][i].valueTypes;
    		nGpios = data["sensors"][i].numberOfGpios;
    		gpiosNames = data["sensors"][i].gpioNames;
    		nParameters = data["sensors"][i].numberOfParameters;
    		parametersNames = data["sensors"][i].parameterNames;
    		parametersTypes = data["sensors"][i].parameterTypes;
    		
    		auxGpiosNames = "";
    		for(let j = 0; j < nGpios; j++){
    			if(j > 0)
    				auxGpiosNames += ",";
    			auxGpiosNames += gpiosNames[j];
    		}
    		
    		auxParametersNames = "";
    		for(let j = 0; j < nParameters; j++){
    			if(j > 0)
    				auxParametersNames += ",";
    			auxParametersNames += parametersNames[j];
    		}
    		
    		auxParametersTypes = "";
    		for(let j = 0; j < nParameters; j++){
    			if(j > 0)
    				auxParametersTypes += ",";
    			auxParametersTypes += parametersTypes[j];
    		}
    		
    		// Generate sensor div
    		$('#sensor_data').append(	'<div class="sensor" id="sensor_'+ i +'" data-sensor_name="'+ name +'" data-sensor_n_units="'+ nUnits +'" data-sensor_n_values="'+ nValues +'" data-sensor_n_gpios="'+ nGpios +'" data-sensor_gpios_names="'+ auxGpiosNames +'" data-sensor_n_parameters="'+ nParameters +'" data-sensor_parameters_names="'+ auxParametersNames +'" data-sensor_parameters_types="'+ auxParametersTypes +'" style="display: none;">\
    									</div>');
    		
    		// Generate sensor header div
    		$('#sensor_'+ i).append(	'<div class="sensor_header" id="sensor_'+ i +'_header">\
    									</div>');
    		
    		// Generate sensor list div
    		$('#sensor_'+ i).append(	'<div class="sensor_list" id="sensor_'+ i +'_list">\
    									</div>');
    		
    		// Generate sensor alerts div
    		$('#sensor_'+ i).append(	'<div class="sensor_alerts" id="sensor_'+ i +'_alerts" hidden>\
    		    						</div>');
    		
    		// Generate sensor functionalities div
    		$('#sensor_'+ i).append(	'<div class="sensor_functionalities" id="sensor_'+ i +'_functionalities">\
										</div>');
    		
			// Header
			$('#sensor_'+ i +'_header').append(	'<div class="row">\
													<div class="col-2">\
														<h4>'+ name +' readings</h4>\
													</div>\
													<div class="col-9">\
													</div>\
													<div class="col-1">\
														<input type="button" value="Delete all units" onclick="deleteAllSensorUnits('+ i +');">\
													</div>\
												</div>');
			
			// Sensor list
    		for(let j = 0; j < nUnits; j++){
    			
    			// Generate sensor unit div
    			$('#sensor_'+ i +'_list').append(	'<div class="sensor_unit" id="sensor_'+ i +'_unit_'+ j +'">\
													</div>');
    			
    			//Generate sensor unit header div
    			$('#sensor_'+ i +'_unit_'+ j).append(	'<div class="sensor_unit_header" id="sensor_'+ i +'_unit_'+ j +'_header">\
    													</div>');
    			
    			//Generate sensor unit values div
    			$('#sensor_'+ i +'_unit_'+ j).append(	'<div class="sensor_unit_values" id="sensor_'+ i +'_unit_'+ j +'_values">\
    			    									</div>');
    			
    			// Unit header
    			$('#sensor_'+ i +'_unit_'+ j +'_header').append(	'<div class="row">\
																		<div class="col-1">\
																			<p>[ Unit '+ j +' ]</p>\
																		</div>\
																		<div class="col-3">\
																			<label>Location: <input type="text" id="sensor_'+ i +'_unit_'+ j +'_location" value="" disabled/></label>\
																			<input type="button" value="Cancel" id="sensor_'+ i +'_unit_'+ j +'_location_cancel" onclick="submitSensorLocation('+ i +','+ j +',false);" hidden>\
																			<input type="button" value="Submit" id="sensor_'+ i +'_unit_'+ j +'_location_submit" onclick="submitSensorLocation('+ i +','+ j +',true);" hidden>\
																			<input type="button" class="edit_location" value="Edit" id="sensor_'+ i +'_unit_'+ j +'_location_edit" onclick="editSensorLocation('+ i +','+ j +');">\
																		</div>\
																		<div class="col-3" id="sensor_'+ i +'_unit_'+ j +'_gpios">\
																		</div>\
																		<div class="col-4" id="sensor_'+ i +'_unit_'+ j +'_parameters">\
																		</div>\
																		<div class="col-1">\
																			<input type="button" value="Delete unit" onclick="deleteSensorUnit('+ i +', '+ j +');">\
																		</div>\
																	</div>');
    			
    			
    			// Fill GPIOS (if there is any)
    			if(nGpios > 0){
    				$('#sensor_'+ i +'_unit_'+ j +'_gpios').append('<p>GPIOS: [ <span id="sensor_'+ i +'_unit_'+ j +'_gpios_span"></span> ]</p>');
    				
    				for(let k = 0; k < nGpios; k++){
    					$('#sensor_'+ i +'_unit_'+ j +'_gpios_span').append(gpiosNames[k] + ': <span id ="sensor_'+ i +'_unit_'+ j +'_gpio_'+ k +'_span"></span>');
    					if(k < nGpios - 1)
    						$('#sensor_'+ i +'_unit_'+ j +'_gpios_span').append(', ');
    				}
    				
    				$('#sensor_'+ i +'_unit_'+ j +'_gpios').append(	'<input type="button" value="Cancel" id="sensor_'+ i +'_unit_'+ j +'_gpios_cancel" onclick="submitSensorGpios('+ i +','+ j +',false);" hidden>\
    																<input type="button" value="Submit" id="sensor_'+ i +'_unit_'+ j +'_gpios_submit" onclick="submitSensorGpios('+ i +','+ j +',true);" hidden>\
    																<input type="button" class="edit_gpios" value="Edit" id="sensor_'+ i +'_unit_'+ j +'_gpios_edit" onclick="editSensorGpios('+ i +','+ j +');">');
    			}
    			
				// Fill parameters (if there is any)
    			if(nParameters > 0){
    				$('#sensor_'+ i +'_unit_'+ j +'_parameters').append('<p>Parameters: [ <span id="sensor_'+ i +'_unit_'+ j +'_parameters_span"></span> ]</p>');
    				
    				for(let k = 0; k < nParameters; k++){
    					$('#sensor_'+ i +'_unit_'+ j +'_parameters_span').append(parametersNames[k] + ': <span id ="sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span" data-parameter_type="'+ parametersTypes[k] +'"></span>');
    					if(k < nParameters - 1)
    						$('#sensor_'+ i +'_unit_'+ j +'_parameters_span').append(', ');
    				}
    				
    				$('#sensor_'+ i +'_unit_'+ j +'_parameters').append(	'<input type="button" value="Cancel" id="sensor_'+ i +'_unit_'+ j +'_parameters_cancel" onclick="submitSensorParameters('+ i +','+ j +',false);" hidden>\
    																		<input type="button" value="Submit" id="sensor_'+ i +'_unit_'+ j +'_parameters_submit" onclick="submitSensorParameters('+ i +','+ j +',true);" hidden>\
																			<input type="button" class="edit_parameters" value="Edit" id="sensor_'+ i +'_unit_'+ j +'_parameters_edit" onclick="editSensorParameters('+ i +','+ j +');">');
    			}
    			
    			// Unit values
    			for(let k = 0; k < nValues; k++){
    				
    				// Generate unit value block div
    				$('#sensor_'+ i +'_unit_'+ j +'_values').append(	'<div class="value_block calm" id="sensor_'+ i +'_unit_'+ j +'_value_block_'+ k +'">\
																		</div>');
    				
    				// Value block
    				$('#sensor_'+ i +'_unit_'+ j +'_value_block_'+ k).append(	'<div class="row">\
																					<div class="col-12">\
																						<p>'+ valuesNames[k] +': <span id="sensor_'+ i +'_unit_'+ j +'_value_'+ k +'_span" data-value_type="'+ valuesTypes[k] +'"></span></p>\
																					</div>\
																				</div>');
    			}
    		}
    		
    		// Sensor alerts
    		for(let j = 0; j < nValues; j++){
    			
    			// Add alert variables of a value
    			$('#sensor_'+ i +'_alerts').append(	'<div class="row">\
														<div class="col-1 alert_value_name">\
															<p>'+ valuesNames[j] +':</p>\
														</div>\
														<div class="col-2">\
															<p>Alert: <span id="sensor_'+ i +'_value_'+ j +'_alert_span"></span></p>\
														</div>\
														<div class="col-2">\
															<p>Ticks: <span id="sensor_'+ i +'_value_'+ j +'_ticks_span"></span></p>\
														</div>\
														<div class="col-3">\
															<p>Upper threshold: <span id="sensor_'+ i +'_value_'+ j +'_upper_threshold_span"></span></p>\
														</div>\
														<div class="col-3">\
															<p>Lower threshold: <span id="sensor_'+ i +'_value_'+ j +'_lower_threshold_span"></span></p>\
														</div>\
														<div class="col-1">\
															<input type="button" value="Cancel" id="sensor_'+ i +'_value_'+ j +'_alerts_cancel" onclick="submitSensorAlerts('+ i +','+ j +',false);" hidden>\
															<input type="button" value="Submit" id="sensor_'+ i +'_value_'+ j +'_alerts_submit" onclick="submitSensorAlerts('+ i +','+ j +',true);" hidden>\
															<input type="button" class="edit_alerts" value="Edit" id="sensor_'+ i +'_value_'+ j +'_alerts_edit" onclick="editSensorAlerts('+ i +','+ j +');">\
														</div>\
													</div>');
    		}
    		
    		// Sensor alerts functionalities
    		$('#sensor_'+ i +'_functionalities').append(	'<div class="row">\
																<div class="col-12">\
																	<input type="button" id="sensor_'+ i +'_alerts_button" value="Show alerts" onclick="triggerAlertsDiv('+ i +');">\
																</div>\
															</div>');
    		
    		if(nUnits > 0)
    			document.getElementById("sensor_" + i).style.display = "block";
    	}
    });
}

/**
 * Update GPIOS availables
 */
function gpiosAvailablesUpdate(){
	
	$.getJSON('/GetGpios', function(data){
		
		gpioOptions.splice(1);
		
		for(let i = 0; i < data["numberOfGpiosAvailables"]; i++){
			gpioOptions[i+1] = data["gpios"][i];
		}
		
		gpioOptions.sort(function(a, b){return a - b});
	});
}

/**
 * Update sensors values
 */
function sensorsValuesUpdate(){
	
	$.getJSON('/SensorsValues', function(data){
		
		var sensor_n_units, sensor_n_values; 
		
		for(let i = 0; i < sensorsInstalled; i++){
			
			sensor_n_units = document.getElementById("sensor_" + i).dataset.sensor_n_units;
			sensor_n_values = document.getElementById("sensor_" + i).dataset.sensor_n_values;
			
			for(let j = 0; j < sensor_n_units; j++){
				for(let k = 0; k < sensor_n_values; k++){
					$('#sensor_'+ i +'_unit_'+ j +'_value_'+ k +'_span').text(data[i.toString()][j][k]);
					
					if(i != 0){
						if(document.getElementById('sensor_'+ i +'_unit_'+ j +'_value_'+ k +'_span').dataset.value_type == "INTEGER"){
							if(parseInt(document.getElementById('sensor_'+ i +'_value_'+ k +'_upper_threshold_span').innerHTML) < data[i.toString()][j][k] || parseInt(document.getElementById('sensor_'+ i +'_value_'+ k +'_lower_threshold_span').innerHTML) > data[i.toString()][j][k])
								document.getElementById('sensor_'+ i +'_unit_'+ j +'_value_block_'+ k).className = "value_block alert";
							else
								document.getElementById('sensor_'+ i +'_unit_'+ j +'_value_block_'+ k).className = "value_block calm";
						}
						else if(document.getElementById('sensor_'+ i +'_unit_'+ j +'_value_'+ k +'_span').dataset.value_type == "FLOAT"){
							if(parseFloat(document.getElementById('sensor_'+ i +'_value_'+ k +'_upper_threshold_span').innerHTML) < data[i.toString()][j][k] || parseFloat(document.getElementById('sensor_'+ i +'_value_'+ k +'_lower_threshold_span').innerHTML) > data[i.toString()][j][k])
								document.getElementById('sensor_'+ i +'_unit_'+ j +'_value_block_'+ k).className = "value_block alert";
							else
								document.getElementById('sensor_'+ i +'_unit_'+ j +'_value_block_'+ k).className = "value_block calm";
						}
						else{
							if(document.getElementById('sensor_'+ i +'_value_'+ k +'_upper_threshold_span').innerHTML === data[i.toString()][j][k] || document.getElementById('sensor_'+ i +'_value_'+ k +'_lower_threshold_span').innerHTML === data[i.toString()][j][k])
								document.getElementById('sensor_'+ i +'_unit_'+ j +'_value_block_'+ k).className = "value_block alert";
							else
								document.getElementById('sensor_'+ i +'_unit_'+ j +'_value_block_'+ k).className = "value_block calm";
						}
					}
				}
			}
		}
	});
}

/**
 * Update sensors GPIOS
 */
function sensorsGpiosUpdate(){
	
	$.getJSON('/SensorsGpios', function(data){
		
		var sensor_n_units, sensor_n_gpios; 
		
		for(let i = 1; i < sensorsInstalled; i++){
			
			sensor_n_units = document.getElementById("sensor_" + i).dataset.sensor_n_units;
			sensor_n_gpios = document.getElementById("sensor_" + i).dataset.sensor_n_gpios;
			
			for(let j = 0; j < sensor_n_units; j++){
				for(let k = 0; k < sensor_n_gpios; k++){
					$('#sensor_'+ i +'_unit_'+ j +'_gpio_'+ k +'_span').text(data[i.toString()][j][k]);
				}
			}
		}
	});
}

/**
 * Update sensors parameters
 */
function sensorsParametersUpdate(){
	
	$.getJSON('/SensorsParameters', function(data){
		
		var sensor_n_units, sensor_n_parameters;
		
		for(let i = 1; i < sensorsInstalled; i++){
			
			sensor_n_units = document.getElementById("sensor_" + i).dataset.sensor_n_units;
			sensor_n_parameters = document.getElementById("sensor_" + i).dataset.sensor_n_parameters;
			
			for(let j = 0; j < sensor_n_units; j++){
				for(let k = 0; k < sensor_n_parameters; k++){
					$('#sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').text(data[i.toString()][j][k]);
				}
			}
		}
	});
}

/**
 * Update sensors locations
 */
function sensorsLocationsUpdate(){
	
	$.getJSON('/SensorsLocations', function(data){
		
		var sensor_n_units;
		
		for(let i = 1; i < sensorsInstalled; i++){
			
			sensor_n_units = document.getElementById("sensor_" + i).dataset.sensor_n_units;
			
			for(let j = 0; j < sensor_n_units; j++){
				$('#sensor_'+ i +'_unit_'+ j +'_location').val(data[i.toString()][j]);
			}
		}
	});
}

/**
 * Update sensors alerts
 */
function sensorsAlertsUpdate(){
	
	$.getJSON('/SensorsAlerts', function(data){
		
		var sensor_n_values;
		
		for(let i = 1; i < sensorsInstalled; i++){
			
			sensor_n_values = document.getElementById("sensor_" + i).dataset.sensor_n_values;
			
			for(let j = 0; j < sensor_n_values; j++){
				$('#sensor_'+ i +'_value_'+ j +'_alert_span').text(data[i.toString()][j][0]);
				$('#sensor_'+ i +'_value_'+ j +'_ticks_span').text(data[i.toString()][j][1]);
				$('#sensor_'+ i +'_value_'+ j +'_upper_threshold_span').text(data[i.toString()][j][2]);
				$('#sensor_'+ i +'_value_'+ j +'_lower_threshold_span').text(data[i.toString()][j][3]);
			}
		}
	});
}

/**
 * Set "sensorsValuesUpdate" as an automatized requested function
 */
function startSensorsValuesUpdateInterval(){
	setInterval(sensorsValuesUpdate, 10000);
}

/**
 * Deploy the form to create the selected sensor.
 */
function deploySensorForm(id){
	
	var name, n_gpios, gpios_names, n_parameters, parameters_names, parameters_types;
	
	name = document.getElementById("sensor_" + id).dataset.sensor_name;
	n_gpios = document.getElementById("sensor_" + id).dataset.sensor_n_gpios;
	gpios_names = document.getElementById("sensor_" + id).dataset.sensor_gpios_names.split(",");
	n_parameters = document.getElementById("sensor_" + id).dataset.sensor_n_parameters;
	parameters_names = document.getElementById("sensor_" + id).dataset.sensor_parameters_names.split(",");
	parameters_types = document.getElementById("sensor_" + id).dataset.sensor_parameters_types.split(",");
	
	// Generate sensor form div
	document.getElementById("sensor_deployment_body").innerHTML =	'<div id="sensor_form">\
																	</div>';
	
	// Add name
	document.getElementById("sensor_form").innerHTML += '<h2>'+ name +'</h2>';
	
	// Add GPIOS
	if(n_gpios > 0)
		document.getElementById("sensor_form").innerHTML += '<h4>----------- GPIOS -----------</h4>';
	for(let i = 0; i < n_gpios; i++){
		document.getElementById("sensor_form").innerHTML +=	'<div class="form_row">\
																<label for="gpio_'+ i +'">'+ gpios_names[i] +':</label>\
																<select name="gpios_options" id="gpio_'+ i +'"></select>\
															</div>';
	}
	
	// Add parameters
	if(n_parameters > 0)
		document.getElementById("sensor_form").innerHTML += '<h4>----- PARAMETERS -----</h4>';
	for(let i = 0; i < n_parameters; i++){
		if(parameters_types[i] == "INTEGER"){
			document.getElementById("sensor_form").innerHTML += '<div class="form_row">\
																	<label for="parameter_'+ i +'">'+ parameters_names[i] +':</label>\
																	<input type="number" name="parameters_options" id="parameter_'+ i +'" value="0">\
																	<input type="hidden" id="parameter_'+ i +'_type" name="parameter_'+ i +'_type" value="INTEGER">\
																</div>';
		}
		else if(parameters_types[i] == "FLOAT"){
			document.getElementById("sensor_form").innerHTML += '<div class="form_row">\
																	<label for="parameter_'+ i +'">'+ parameters_names[i] +':</label>\
																	<input type="number" name="parameters_options" id="parameter_'+ i +'" value="0" step="0.01">\
																	<input type="hidden" id="parameter_'+ i +'_type" name="parameter_'+ i +'_type" value="FLOAT">\
																</div>';
		}
		else{
			document.getElementById("sensor_form").innerHTML += '<div class="form_row">\
																	<label for="parameter_'+ i +'">'+ parameters_names[i] +':</label>\
																	<input type="text" name="parameters_options" id="parameter_'+ i +'">\
																	<input type="hidden" id="parameter_'+ i +'_type" name="parameter_'+ i +'_type" value="STRING">\
																</div>';
		}
	}
	
	// Fill GPIOS options
	fillAndUpdateOptions();
	
	// Put the buttons
	document.getElementById("sensor_form").innerHTML += '<div class="form_row">\
															<input type="button" value="Cancel" onclick="processData(-1);">\
															<input type="button" value="Submit" onclick="processData('+ id +');">\
														</div>';
	
	// Generate additional info divs
	document.getElementById("sensor_deployment_body").innerHTML +=	'<div id="gpios_notes">\
																	</div>\
																	<div id="warning_notes">\
																	</div>';
	
	// Deploy some gpios notes
	document.getElementById("gpios_notes").innerHTML =	'<h4>GPIOS (ESP32 Wrover-DevKit):</h4>\
														<ul>\
															<li>ADC1: 32 (CH4) || 33 (CH5) || 34 (CH6) || 35 (CH7) || 36 (CH0) || 39 (CH3)</li>\
															<li>ADC2: 0 (CH1) || 2 (CH2) || 4 (CH0) || 12 (CH5) || 13 (CH4) || 14 (CH6) || 15 (CH3) || 25 (CH8) || 26 (CH9) || 27 (CH7)</li>\
															<li>VSPI: 5 (CS) || 18 (CLK) || 19 (MISO) || 23 (MOSI)</li>\
															<li>HSPI: 12 (MISO) || 13 (MOSI) || 14 (CLK) || 15 (CS)</li>\
															<li>DEEP SLEEP: 0 (RTC11) || 2 (RTC12) || 4 (RTC10) || 12 (RTC15) || 13 (RTC14) || 14 (RTC16) || 15 (RTC13) || 25 (RTC6) || 26 (RTC7) || 27 (RTC17) || 32 (RTC9) || 33 (RTC8) || 34 (RTC4) || 35 (RTC5) || 36 (RTC0) || 39 (RTC3)</li>\
															<li>TOUCH SENS: 0 (CH1) || 2 (CH2) || 4 (CH0) || 12 (CH5) || 13 (CH4) || 14 (CH6) || 15 (CH3) || 27 (CH7) || 32 (CH9) || 33 (CH8)</li>\
															<li>I2C: 21 (SDA) || 22 (SCL)</li>\
															<li>DAC: 25 (CH1) || 26 (CH2)</li>\
															<li>SD: 2 (DAT0) || 4 (DAT1) || 12 (DAT2) || 13 (DAT3) || 14 (CLK) || 15 (CMD)</li>\
															<li>UART: 1 (TX0) || 3 (RX0)</li>\
														</ul>';
	
	
	// Deploy some warning notes
	document.getElementById("warning_notes").innerHTML =	'<h4>Warnings:</h4>\
															<ul>\
																<li>GPIO 12 is internally pulled high in the module and is not recommended for use as a touch pin.</li>\
																<li>GPIOS 6,7,8,9,10,11 are connected to the SPI flash integrated on the module and are not recommended for other uses.</li>\
																<li>ADC2 channels shouldn\'t be used while ESP32 WiFi function is active. (Values obtained will be incorrect)</li>\
																<li>GPIOS 0,2,5,12,15 are strapping pins.</li>\
																<li>GPIOS 12,13,14,15 are usually used for inline debug.</li>\
																<li>GPIOS 34,35,36,39 can only be set as input mode and do not support pullup or pulldown functions.</li>\
																<li>GPIOS 1,3 are usually used for flashing and debugging.</li>\
																<li>Do not use GPIOS 36,39 interrupts when using ADC or WiFi and Bluetooth with sleep mode enabled.</li>\
															</ul>';
}

/**
 * Process the data to create the selected sensor.
 */
function processData(id){
	
	if(id != -1){
		
		var nGpios = document.getElementById("sensor_" + id).dataset.sensor_n_gpios;
		var nParameters = document.getElementById("sensor_" + id).dataset.sensor_n_parameters;
		var k = 0, ok = true;
		
		while(k < nGpios && ok){
			if(document.getElementById("gpio_"+ k).value == 0){
				ok = false;
				window.alert('Error, don\'t let any GPIO selected as "None"');
			}
			else
				k++;
		}
		
		k = 0;
		
		while(k < nParameters && ok){
			if(document.getElementById("parameter_"+ k).value.length == 0){
				ok = false;
				window.alert('Error, don\'t let any parameters empty');
			}
			else
				k++;
		}
		
		if(ok)
			addSensorUnit(id);
	}
	
	clearDiv("sensor_deployment_body");
}

function addSensorUnit(id){
	
	var nGpios, nParameters;
	var gpios = "", parameters = "";
	
	nGpios = document.getElementById("sensor_" + id).dataset.sensor_n_gpios;
	nParameters = document.getElementById("sensor_" + id).dataset.sensor_n_parameters;
	
	for(let i = 0; i < nGpios; i++){
		gpios += gpioOptions[document.getElementById("gpio_"+ i).value] + "\n";
	}
	
	for(let i = 0; i < nParameters; i++){
		parameters += document.getElementById("parameter_"+ i + "_type").value + "\n";
		parameters += document.getElementById("parameter_"+ i).value + "\n";
	}
		
	// Http Request
	var request = new XMLHttpRequest();
	var requestURL = "/SensorUnitAddition";		//SensorUnitAddition has to be handled
	
	request.open('POST', requestURL, false);
	
	request.send(nGpios + "\n" + gpios + nParameters + "\n" + parameters + id + "\0");
    
    if (request.readyState == 4 && request.status == 200) {
    	window.alert(request.responseText);
    }
	
    clearDiv("sensor_data");
	initSensors();
}

function deleteSensorUnit(i,j)
{
	// Http Request
	var request = new XMLHttpRequest();
	var requestURL = "/SensorUnitDeletion";		//SensorUnitDeletion has to be handled
	
	request.open('POST', requestURL, false);
	
	request.send(i + "\n" + j + "\0");
    
    if (request.readyState == 4 && request.status == 200) {
    	window.alert(request.responseText);
    }
	
	clearDiv("sensor_data");
	initSensors();
}

function deleteAllSensorUnits(i)
{
	var request;
	var requestURL;
	var nUnits = document.getElementById("sensor_" + i).dataset.sensor_n_units;
	
	for(let j = nUnits - 1; j >= 0; j--){
		// Http Request
		request = new XMLHttpRequest();
		requestURL = "/SensorUnitDeletion";		//SensorUnitDeletion has to be handled
		
		request.open('POST', requestURL, false);
		
		request.send(i + "\n" + j + "\0");
	}
	
	window.alert('All sensor units deleted');
	
	clearDiv("sensor_data");
	initSensors();
}

function triggerAlertsDiv(i){
	if(document.getElementById('sensor_'+ i +'_alerts').hidden == true){
		document.getElementById('sensor_'+ i +'_alerts').hidden = false;
		document.getElementById('sensor_'+ i +'_alerts_button').value = "Hide alerts";
	}
	else{
		document.getElementById('sensor_'+ i +'_alerts').hidden = true;
		document.getElementById('sensor_'+ i +'_alerts_button').value = "Show alerts";
	}
}

function submitWifiConf()
{
	var xhr = new XMLHttpRequest();
    var requestURL = "/setWIFIConfiguration";		//setWIFIConfiguration has to be handled
    xhr.open('POST', requestURL, false);
    xhr.send(document.getElementById('WIFI_SSID').value + "\n" 
    		+ document.getElementById('WIFI_PASS').value + "\0");
    
    if (xhr.readyState == 4 && xhr.status == 200) {
    	window.alert(xhr.responseText);
    }else window.alert("Something went wrong!");
    
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

function submitNTPConf()
{
	//window.alert("NTP Server: " + document.getElementById('NTP_SERVER').value + "\n" + "Sync time (secs): " + document.getElementById('NTP_SYNC').value + "\n");
	
    var xhr = new XMLHttpRequest();
    var requestURL = "/setNTPConfiguration";		//setNTPConfiguration has to be handled
    xhr.open('POST', requestURL, false);
    xhr.send(document.getElementById('NTP_SERVER').value + "\n" + document.getElementById('NTP_SYNC').value + "\0");
    
    if (xhr.readyState == 4 && xhr.status == 200) {
    	window.alert(xhr.responseText);
    }
    
	document.getElementById('NTP_SERVER').disabled = true;
	document.getElementById("NTP_SYNC").disabled = true;
	document.getElementById('NTP_EDIT').hidden = false;
	document.getElementById('NTP_SUBMIT').hidden = true;
	getConnectionsConfiguration();
}

function editNTPConf()
{
	document.getElementById('NTP_SERVER').disabled = false;
	document.getElementById("NTP_SYNC").disabled = false;
	document.getElementById('NTP_EDIT').hidden = true;
	document.getElementById('NTP_SUBMIT').hidden = false;
	getConnectionsConfiguration();
}

function submitMQTTConf()
{
	var xhr = new XMLHttpRequest();
    var requestURL = "/setMQTTConfiguration";		//setMQTTConfiguration has to be handled
    xhr.open('POST', requestURL, false);
    xhr.send(document.getElementById('MQTT_IP').value + "\n" 
    		+ document.getElementById('MQTT_PORT').value + "\n"
    		+ document.getElementById('MQTT_USER').value + "\n"
    		+ document.getElementById('MQTT_PASS').value + "\0");
    
    if (xhr.readyState == 4 && xhr.status == 200) {
    	window.alert(xhr.responseText);
    }
    
	document.getElementById('MQTT_IP').disabled = true;
	document.getElementById('MQTT_PORT').disabled = true;
	document.getElementById('MQTT_USER').disabled = true;
	document.getElementById('MQTT_PASS').disabled = true;
	document.getElementById('MQTT_EDIT').hidden = false;
	document.getElementById('MQTT_SUBMIT').hidden = true;
	getConnectionsConfiguration();
}

function editMQTTConf()
{
	document.getElementById('MQTT_IP').disabled = false;
	document.getElementById('MQTT_PORT').disabled = false;
	document.getElementById('MQTT_USER').disabled = false;
	document.getElementById('MQTT_PASS').disabled = false;
	document.getElementById('MQTT_EDIT').hidden = true;
	document.getElementById('MQTT_SUBMIT').hidden = false;
	getConnectionsConfiguration();
}

function submitSensorLocation(i,j,process)
{
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_location').disabled = true;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_location_edit').hidden = false;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_location_cancel').hidden = true;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_location_submit').hidden = true;
	
	if(process == true){
		// Http Request
		var request = new XMLHttpRequest();
		var requestURL = "/SensorEditLocation";		//SensorEditLocation has to be handled
		var loc = document.getElementById('sensor_'+ i +'_unit_'+ j +'_location').value;
		
		request.open('POST', requestURL, false);
		
		request.send(i + "\n" + j + "\n" + loc.length + "\n" + loc + "\0");
		
		if (request.readyState == 4 && request.status == 200) {
			window.alert(request.responseText);
		}
	}
	
	unlockEdits("edit_location");
	sensorsLocationsUpdate();
}

function editSensorLocation(i,j)
{
	lockEdits("edit_location");
	
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_location').disabled = false;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_location_edit').hidden = true;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_location_cancel').hidden = false;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_location_submit').hidden = false;
}

function submitSensorGpios(i,j,process)
{
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpios_edit').hidden = false;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpios_cancel').hidden = true;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpios_submit').hidden = true;
	
	if(process == true){
		
		var nGpios = document.getElementById("sensor_"+ i).dataset.sensor_n_gpios;
		var gpios = "";

		var x = 0, ok = true;
		
		while(x < nGpios && ok){
			if(document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpio_'+ x +'_selected').value == 0){
				ok = false;
				window.alert('Error, don\'t let any GPIO selected as "None"');
			}
			else
				x++;
		}
		
		if(ok){
			for(let k = 0; k < nGpios; k++){
				gpios += gpioOptions[document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpio_'+ k +'_selected').value] + "\n";
			}
				
			// Http Request
			var request = new XMLHttpRequest();
			var requestURL = "/SensorEditGpios";		//SensorEditGpios has to be handled
			
			request.open('POST', requestURL, false);
			
			request.send(nGpios + "\n" + gpios + i + "\n" + j + "\0");
			
			if (request.readyState == 4 && request.status == 200) {
				window.alert(request.responseText);
			}
			
			gpiosAvailablesUpdate();
			setTimeout(fillAndUpdateOptions, 500);
		}
	}
	
	unlockEdits("edit_gpios");
	sensorsGpiosUpdate();
}

function editSensorGpios(i,j)
{
	lockEdits("edit_gpios");
			
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpios_edit').hidden = true;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpios_cancel').hidden = false;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpios_submit').hidden = false;
	
	var nGpios = document.getElementById("sensor_" + i).dataset.sensor_n_gpios;
	
	for(let k = 0; k < nGpios; k++){
		document.getElementById('sensor_'+ i +'_unit_'+ j +'_gpio_'+ k +'_span').innerHTML = '<select name="gpios_options" id="sensor_'+ i +'_unit_'+ j +'_gpio_'+ k +'_selected"></select>';
	}
	
	// Fill options with all GPIOS availables
	fillAndUpdateOptions();
}

function submitSensorParameters(i,j,process)
{
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameters_edit').hidden = false;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameters_cancel').hidden = true;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameters_submit').hidden = true;
	
	if(process == true){
		var nParameters = document.getElementById("sensor_"+ i).dataset.sensor_n_parameters;
		var parameters = "";
		
		var x = 0, ok = true;
		
		while(x < nParameters && ok){
			if(document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ x +'_selected').value.length == 0){
				ok = false;
				window.alert('Error, don\'t let any parameters empty');
			}
			else
				x++;
		}
		
		if(ok){
			for(let k = 0; k < nParameters; k++){
				parameters += document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').dataset.parameter_type + "\n";
				parameters += document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_selected').value + "\n";
			}
				
			// Http Request
			var request = new XMLHttpRequest();
			var requestURL = "/SensorEditParameters";		//SensorEditParameters has to be handled
			
			request.open('POST', requestURL, false);
			
			request.send(nParameters + "\n" + parameters + i + "\n" + j + "\0");
			
			if (request.readyState == 4 && request.status == 200) {
				window.alert(request.responseText);
			}
		}
	}
	
	unlockEdits("edit_parameters");
	sensorsParametersUpdate();
}

function editSensorParameters(i,j)
{
	lockEdits("edit_parameters");
	
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameters_edit').hidden = true;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameters_cancel').hidden = false;
	document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameters_submit').hidden = false;
	
	var nParameters = document.getElementById("sensor_" + i).dataset.sensor_n_parameters;
	
	for(let k = 0; k < nParameters; k++){
		if(document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').dataset.parameter_type == "INTEGER")
			document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').innerHTML = '<input type="number" name="sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_selected" id="sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_selected" value="'+ document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').innerHTML +'">';
		else if(document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').dataset.parameter_type == "FLOAT")
			document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').innerHTML = '<input type="number" name="sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_selected" id="sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_selected" value="'+ document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').innerHTML +'" step="0.01">';
		else
			document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').innerHTML = '<input type="text" name="sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_selected" id="sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_selected" value="'+ document.getElementById('sensor_'+ i +'_unit_'+ j +'_parameter_'+ k +'_span').innerHTML +'">';
	}
}

function submitSensorAlerts(i,j,process)
{
	document.getElementById('sensor_'+ i +'_value_'+ j +'_alerts_edit').hidden = false;
	document.getElementById('sensor_'+ i +'_value_'+ j +'_alerts_cancel').hidden = true;
	document.getElementById('sensor_'+ i +'_value_'+ j +'_alerts_submit').hidden = true;
	
	if(process == true){
		var alert = $('input[name="sensor_'+ i +'_value_'+ j +'_alert"]:checked').val();
		var ticks = document.getElementById('sensor_'+ i +'_value_'+ j +'_ticks_selected').value;
		var thresholdType = document.getElementById('sensor_'+ i +'_unit_0_value_'+ j +'_span').dataset.value_type;
		var upperThreshold = document.getElementById('sensor_'+ i +'_value_'+ j +'_upper_threshold_selected').value;
		var lowerThreshold = document.getElementById('sensor_'+ i +'_value_'+ j +'_lower_threshold_selected').value;
		
		var ok = true;
		
		if(ticks.length == 0 || upperThreshold.length == 0 || lowerThreshold.length == 0){
			ok = false;
			window.alert('Error, don\'t let any values empty');
		}
		
		if(ok){
			// Http Request
			var request = new XMLHttpRequest();
			var requestURL = "/SensorEditAlerts";		//SensorEditAlerts has to be handled
			
			request.open('POST', requestURL, false);
			
			request.send(i + "\n" + j + "\n" + alert + "\n" + ticks + "\n" + thresholdType + "\n" + upperThreshold + "\n" + lowerThreshold + "\0");
			
			if (request.readyState == 4 && request.status == 200) {
				window.alert(request.responseText);
			}
		}
	}
	
	unlockEdits("edit_alerts");
	sensorsAlertsUpdate();
}

function editSensorAlerts(i,j)
{
	lockEdits("edit_alerts");
	
	document.getElementById('sensor_'+ i +'_value_'+ j +'_alerts_edit').hidden = true;
	document.getElementById('sensor_'+ i +'_value_'+ j +'_alerts_cancel').hidden = false;
	document.getElementById('sensor_'+ i +'_value_'+ j +'_alerts_submit').hidden = false;
	
	if(document.getElementById('sensor_'+ i +'_value_'+ j +'_alert_span').innerHTML == "false"){
		document.getElementById('sensor_'+ i +'_value_'+ j +'_alert_span').innerHTML = 	'<label for="sensor_'+ i +'_value_'+ j +'_alert_selected_true">True</label> <input type="radio" id="sensor_'+ i +'_value_'+ j +'_alert_selected_true" name="sensor_'+ i +'_value_'+ j +'_alert" value="True">\
																						<label for="sensor_'+ i +'_value_'+ j +'_alert_selected_false">False</label> <input type="radio" id="sensor_'+ i +'_value_'+ j +'_alert_selected_false" name="sensor_'+ i +'_value_'+ j +'_alert" value="False" checked>';
	}
	else{
		document.getElementById('sensor_'+ i +'_value_'+ j +'_alert_span').innerHTML = 	'<label for="sensor_'+ i +'_value_'+ j +'_alert_selected_true">True</label> <input type="radio" id="sensor_'+ i +'_value_'+ j +'_alert_selected_true" name="sensor_'+ i +'_value_'+ j +'_alert" value="True"  checked>\
																						<label for="sensor_'+ i +'_value_'+ j +'_alert_selected_false">False</label> <input type="radio" id="sensor_'+ i +'_value_'+ j +'_alert_selected_false" name="sensor_'+ i +'_value_'+ j +'_alert" value="False">';
	}
	
	document.getElementById('sensor_'+ i +'_value_'+ j +'_ticks_span').innerHTML = '<input type="number" name="sensor_'+ i +'_value_'+ j +'_ticks_selected" id="sensor_'+ i +'_value_'+ j +'_ticks_selected" value="'+ document.getElementById('sensor_'+ i +'_value_'+ j +'_ticks_span').innerHTML +'" min="1">';
	
	if(document.getElementById('sensor_'+ i +'_unit_0_value_'+ j +'_span').dataset.value_type == "INTEGER"){
		document.getElementById('sensor_'+ i +'_value_'+ j +'_upper_threshold_span').innerHTML = '<input type="number" name="sensor_'+ i +'_value_'+ j +'_upper_threshold_selected" id="sensor_'+ i +'_value_'+ j +'_upper_threshold_selected" value="'+ document.getElementById('sensor_'+ i +'_value_'+ j +'_upper_threshold_span').innerHTML +'">';
		document.getElementById('sensor_'+ i +'_value_'+ j +'_lower_threshold_span').innerHTML = '<input type="number" name="sensor_'+ i +'_value_'+ j +'_lower_threshold_selected" id="sensor_'+ i +'_value_'+ j +'_lower_threshold_selected" value="'+ document.getElementById('sensor_'+ i +'_value_'+ j +'_lower_threshold_span').innerHTML +'">';
	}
	else if(document.getElementById('sensor_'+ i +'_unit_0_value_'+ j +'_span').dataset.value_type == "FLOAT"){
		document.getElementById('sensor_'+ i +'_value_'+ j +'_upper_threshold_span').innerHTML = '<input type="number" name="sensor_'+ i +'_value_'+ j +'_upper_threshold_selected" id="sensor_'+ i +'_value_'+ j +'_upper_threshold_selected" value="'+ document.getElementById('sensor_'+ i +'_value_'+ j +'_upper_threshold_span').innerHTML +'" step="0.01">';
		document.getElementById('sensor_'+ i +'_value_'+ j +'_lower_threshold_span').innerHTML = '<input type="number" name="sensor_'+ i +'_value_'+ j +'_lower_threshold_selected" id="sensor_'+ i +'_value_'+ j +'_lower_threshold_selected" value="'+ document.getElementById('sensor_'+ i +'_value_'+ j +'_lower_threshold_span').innerHTML +'" step="0.01">';
	}
	else{
		document.getElementById('sensor_'+ i +'_value_'+ j +'_upper_threshold_span').innerHTML = '<input type="text" name="sensor_'+ i +'_value_'+ j +'_upper_threshold_selected" id="sensor_'+ i +'_value_'+ j +'_upper_threshold_selected" value="'+ document.getElementById('sensor_'+ i +'_value_'+ j +'_upper_threshold_span').innerHTML +'">';
		document.getElementById('sensor_'+ i +'_value_'+ j +'_lower_threshold_span').innerHTML = '<input type="text" name="sensor_'+ i +'_value_'+ j +'_lower_threshold_selected" id="sensor_'+ i +'_value_'+ j +'_lower_threshold_selected" value="'+ document.getElementById('sensor_'+ i +'_value_'+ j +'_lower_threshold_span').innerHTML +'">';
	}
}

function fillAndUpdateOptions(){
	
	var options = "";
	
	for(let k = 0; k < gpioOptions.length; k++){
		options += '<option value="'+ k +'">'+ gpioOptions[k] +'</option>';
	}
	
	$('select[name="gpios_options"]').empty().append(options);
}

function restartEsp(){
	
	// Http Request
	var request = new XMLHttpRequest();
	var requestURL = "/RestartESP";		//RestartESP has to be handled
	
	request.open('POST', requestURL);
	
	request.send();
	
	window.location.reload();
}

function ClearAndRestartEsp(){
	
	// Http Request
	var request = new XMLHttpRequest();
	var requestURL = "/ClearAndRestartESP";		//ClearAndRestartESP has to be handled
	
	request.open('POST', requestURL);
	
	request.send();
}

function changeTab(number){
	
	unlockTab();
			
	if(number == 0){
		$('#status_button').attr('disabled', true);
		document.getElementById('sensor_0').style.display = "block";
	}
	else if(number == 1){
		$('#configurations_button').attr('disabled', true);
		document.getElementById('CONFIGURATIONS').style.display = "block";
	}
	else if(number == 2){
		$('#sensors_button').attr('disabled', true);
		document.getElementById('SENSORS').style.display = "block";
	}
	else if(number == 3){
		$('#ota_button').attr('disabled', true);
		document.getElementById('OTA').style.display = "block";
	}
	
	tab = number;
}

function unlockTab(){
	
	if(tab == 0){
		$('#status_button').attr('disabled', false);
		document.getElementById('sensor_0').style.display = "none";
	}
	else if(tab == 1){
		$('#configurations_button').attr('disabled', false);
		document.getElementById('CONFIGURATIONS').style.display = "none";
	}
	else if(tab == 2){
		$('#sensors_button').attr('disabled', false);
		document.getElementById('SENSORS').style.display = "none";
	}
	else if(tab == 3){
		$('#ota_button').attr('disabled', false);
		document.getElementById('OTA').style.display = "none";
	}
}

function lockEdits(edit_type){
	$('input.'+ edit_type).attr('disabled', true);
}

function unlockEdits(edit_type){
	$('input.'+ edit_type).attr('disabled', false);
}

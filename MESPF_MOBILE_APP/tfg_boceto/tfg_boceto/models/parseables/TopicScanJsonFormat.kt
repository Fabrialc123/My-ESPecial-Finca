package com.example.tfg_boceto.models.parseables


/**
 *
 * Cuando se haga un SCAN se obtendrá lo siguiente de ejemplo:
{
"DT": "ESP32_f867BC",
"TS": "21:44:11 29/12/2022"
}

 */

//TODO MODIFICAR ESTA CLASE PARA LEER LOS DATOS COMO ESTAN EN MQTT EXPLORER
class TopicScanJsonFormat(val DT: String, val ST: String) {
}
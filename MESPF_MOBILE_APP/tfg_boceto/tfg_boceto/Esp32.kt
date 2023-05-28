package com.example.tfg_boceto

data class Esp32 (val nombre_esp: String, val alias_esp: String,var numero:Int, var temperatura: Double, var humedad: Double, var nivel_agua: Double, var concentracion_gas: Double, var humedad_tierra: Double, var last_publish: String) {

}
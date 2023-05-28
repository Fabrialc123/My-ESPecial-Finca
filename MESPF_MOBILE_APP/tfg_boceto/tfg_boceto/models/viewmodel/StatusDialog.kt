package com.example.tfg_boceto.models.viewmodel

import android.app.Dialog
import android.content.Context
import android.os.Bundle
import android.util.Log
import android.view.View
import androidx.transition.Visibility
import com.example.tfg_boceto.R
import com.example.tfg_boceto.databinding.ActivityScanBinding
import com.example.tfg_boceto.databinding.DialogoStatusBinding
import org.json.JSONException
import org.json.JSONObject
import org.json.JSONTokener


private lateinit var binding: DialogoStatusBinding


/**
 *
 * Ejemplo de STATUS:
{
"DT": {
"1": "192.168.68.78",
"2": "ESP32_f867BC",
"3": "21:23:01",
"4": "003 00:19"
},
"TS": "21:23:01 29/12/2022"
}
 *
 *
 */


class StatusDialog(context: Context, private val status: String): Dialog(context) {
    private var ip: String = ""
    private var nombre: String = ""
    private var timeUp: String = ""
    private var horaEnvio: String = ""
    //TODO terminar esta clase
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //bind view to binding e inflarla
        binding = DialogoStatusBinding.inflate(layoutInflater)
        setContentView(binding.root)

        Log.d("STATUS", "PArsea $status")
        if(status != ""){
            parseaStatus(status)
            binding.dialogIp.text = "IP: $ip"
            binding.dialogNombre.text = "Identificador: $nombre"
            binding.dialogHora.text = "Hora del último mensaje: $horaEnvio"
            binding.tiempoActivo.text = "Tiempo Activo días: $timeUp horas"
        }

        else{
            binding.dialogIp.text = "NO HAY COMUNICACIÓN "

            binding.dialogNombre.visibility = View.GONE
            binding.dialogHora.visibility = View.GONE
            binding.tiempoActivo.visibility = View.GONE
        }





    }

    private fun parseaStatus(jsonSinParse: String){
        val jsonObject = JSONTokener(jsonSinParse).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")

        try{
            ip = dt.getString("1")
            nombre = dt.getString("2")
            horaEnvio = dt.getString("3")
            timeUp = dt.getString("4")
        }catch(e:JSONException){}
    }

    override fun onBackPressed() {
        super.onBackPressed()
    }
}
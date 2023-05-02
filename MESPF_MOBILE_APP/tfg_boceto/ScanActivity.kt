package com.example.tfg_boceto

import android.content.ContentValues
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.tfg_boceto.adapter.TopicAdapter
import com.example.tfg_boceto.databinding.ActivityScanBinding
import com.example.tfg_boceto.models.Mqtt
import com.example.tfg_boceto.models.MqttResultado
import com.example.tfg_boceto.models.persistencia.TopicDatabaseHelper
import org.json.JSONException

import org.json.JSONObject
import org.json.JSONTokener


private const val SCAN_CONST = "MESPF/SENS/SCAN"
//AL scan resp hay que añadirle el username/SCAN
private const val SCAN_RESP = "MESPF/USR/"
private const val MQTT_RECIBE = "MESPF/SENS/+/+/+/INFO"
//private const val MQTT_RECIBE = "TESTRARES"
private lateinit var binding: ActivityScanBinding
private var mqttDatos: Mqtt? = null
private val topicMutableList: MutableList<String> = mutableListOf<String>()
private lateinit var adapter: TopicAdapter
private lateinit var elementoElegio: String
private lateinit var usernameLogin: String
private lateinit var passwordLogin: String
private const val DB_NAME = "TOPIC_TABLE"
private const val COL_NAME = "Nombre"
private var SCAN_RESPUESTA = SCAN_RESP+ userName+"/SCAN"

class ScanActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        //bind view to binding e inflarla
        binding = ActivityScanBinding.inflate(layoutInflater)
        setContentView(binding.root)


        //inicia recycler view
        initRecyclerView()
        mqttDatos = Mqtt(MQTT_SERVER)
        //TODO CAMBIAR USUARIO Y CONTRASEÑA DE MANERA DINAMICA
        //usernameLogin = "MESPF_USER"
        //passwordLogin = "MESPF_USER"

        //Get las variables del intent anterior
        val intent = Intent(this, ScanActivity::class.java)

        mqttDatos?.connect(userLogin, userPassword,::suscribesTopic, ::onMqttError)

        Toast.makeText(this, "Espere un momento mientras se buscan dispositivos", Toast.LENGTH_LONG).show()

        binding.scanBtnAdd.setOnClickListener {
            addTopicFunc()
        }
    }

    //SUB TOPICO DEL SCAN
    private fun subTopico(toopico: String) = runOnUiThread{
        mqttDatos?.suscribeTopic(toopico)?.observe(this) { result ->
            when (result) {
                is MqttResultado.Failure -> onMqttError(result)
                is MqttResultado.Success -> onMqttMsgScan(result)
                else -> {}
            }

        }
    }

    private fun parseTopicNotList(msg: String): String{
        if(msg.contains("DT")){
            try{
                val jsonObject = JSONObject(msg)
                val dt = jsonObject.getString("DT")
                return dt
            }catch(e: JSONException){
                Log.d("JSON", "Mensaje no tiene formato correcto $msg")
            }
        }




        return ""

    }

    private fun onMqttMsgScan(result: MqttResultado.Success){
        result?.payload.run {
            if(this == null)
                return
            val nameString = String(this!!)
                if(!nameString.contains("DT"))
                    return
                val topicoN = parseTopicNotList(nameString)


                if(!topicMutableList.contains(topicoN)){
                    topicMutableList.add(topicoN)
                    adapter.notifyItemInserted(adapter.itemCount - 1)
                }
        }
    }

    private fun publishTopic(topic: String, payload: String){
        mqttDatos?.publishTopic(topic, payload)
    }
    private fun onMqttError(failure: MqttResultado.Failure) {
        Toast.makeText(this, "Error connecting to mqtt ", Toast.LENGTH_SHORT).show()

    }

    private fun suscribesTopic() = runOnUiThread {
        mqttDatos?.suscribeTopic(SCAN_RESPUESTA)?.observe(this) { result ->
            when (result) {
                is MqttResultado.Failure -> onMqttError(result)
                is MqttResultado.Success -> onMqttMsgScan(result)
                else -> {}
            }

        }

        publishTopic(SCAN_CONST, userName)
    }




    /**
     *Cuando se haga un SCAN se obtendrá lo siguiente de ejemplo:
    { "DT": "ESP32_f867BC",
    "TS": "21:44:11 29/12/2022"}
     *
     */
    private fun onMqttMessageReceived(result: MqttResultado.Success) {
        result?.payload.run {

            val nameString = String(this!!)
            var topicCurr: String = ""
            topicCurr = mqttData?.getTopic().toString()
            //TODO comprobar si el formato de parseo es correcto
            Toast.makeText(this@ScanActivity, nameString, Toast.LENGTH_SHORT).show()
            if (topicCurr.contains("/USR/$userName/SCAN")) {

                val topicoN = parseTopicNameJSON(nameString)


                if(!topicMutableList.contains(topicoN)){

                    topicMutableList.add(topicoN)
                    adapter.notifyItemInserted(adapter.itemCount - 1)
                }
            }
            Toast.makeText(this@ScanActivity, nameString, Toast.LENGTH_LONG).show()
        }
    }


    private fun parseTopicNameJSON(cadena: String): String {
        val jsonObject = JSONTokener(cadena).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")

        return dt.getString("2")
    }

    private fun initRecyclerView() {
        adapter = TopicAdapter(
            topicList = topicMutableList,
            onClickListener = { topic -> onItemSelected(topic) })

        val manager = LinearLayoutManager(this)

        binding.scanRecyclerList.layoutManager = manager
        binding.scanRecyclerList.adapter = adapter
    }

    //se guarda en la variable el ultimo elemento elegido, deberia verse el color de fondo actualizado de este item
    private fun onItemSelected(topic: String) {
        Toast.makeText(this, "Pulse el boton ADD para añadir el dispositivo", Toast.LENGTH_LONG)
            .show()
        elementoElegio = topic;
    }

    private fun checkTopicExist(topicIn: String): Boolean{
        var topic: String = topicIn
        val dbAct = TopicDatabaseHelper(this)
        val dbR = dbAct.readableDatabase
        val curs = dbR.query(
            DB_NAME,
            arrayOf(COL_NAME),
            "Nombre = ?",
            arrayOf(topic),
            null,
            null,
            null
        )
        val nameExists = curs.count > 0
        curs.close()
        dbR.close()
        return nameExists
    }

    private fun addTopicFunc() {
        mqttDatos?.unsubscribe(MQTT_RECIBE)
        val dbAct = TopicDatabaseHelper(this)

        if(checkTopicExist(elementoElegio)){
            Toast.makeText(this@ScanActivity, "Topico existente ", Toast.LENGTH_SHORT).show()
            return
        }


        val cursor = dbAct
        //Introducimos topic en database
        //TODO insert en el database
        val db = dbAct.writableDatabase
        val contentValues = ContentValues()
        contentValues.put("Nombre", elementoElegio)
        contentValues.put("Alias", elementoElegio) // Al principio nombre y alias iguales
        db.insert("TOPIC_TABLE", null, contentValues)
        db.close()

        //Eliminar elemento de la lista
        val pos = topicMutableList.indexOf(elementoElegio)
        topicMutableList.remove(elementoElegio)
        adapter.notifyItemRemoved(pos)
        Toast.makeText(this@ScanActivity, "Se ha suscrito al dispositivo "+ elementoElegio, Toast.LENGTH_SHORT).show()

    }

    override fun onBackPressed() {
        super.onBackPressed()
        if(mqttDatos?.connected() == true){
            mqttDatos?.unsubscribe(MQTT_RECIBE)
            mqttDatos?.disconnect()
        }


        val intent = Intent(this@ScanActivity, MainActivity::class.java)
        intent.putExtra("vuelta", true)
        startActivity(intent)
    }


}



package com.example.tfg_boceto

import android.content.ContentValues
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Handler
import android.widget.Toast
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.tfg_boceto.adapter.TopicAdapter
import com.example.tfg_boceto.databinding.ActivityScanBinding
import com.example.tfg_boceto.models.Mqtt
import com.example.tfg_boceto.models.MqttResultado
import com.example.tfg_boceto.models.persistencia.TopicDatabaseHelper

import org.json.JSONObject
import org.json.JSONTokener


private const val MQTT_RECIBE = "MESPF/SENS/+/+/+/INFO"
//private const val MQTT_RECIBE = "TESTRARES"
private lateinit var binding: ActivityScanBinding
private var mqttDatos: Mqtt? = null
private val topicMutableList: MutableList<String> = mutableListOf<String>()
private lateinit var adapter: TopicAdapter
private lateinit var elementoElegio: String
private lateinit var usernameLogin: String
private lateinit var passwordLogin: String


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
        usernameLogin = "MESPF_USER"
        passwordLogin = "MESPF_USER"

        //Get las variables del intent anterior
        val intent = Intent(this, ScanActivity::class.java)
        usernameLogin = intent.getStringExtra("username").toString()
        passwordLogin = intent.getStringExtra("password").toString()


        mqttDatos?.connect(usernameLogin, passwordLogin,::suscribesTopic, ::onMqttError)

        //TODO DE PRUEBA POR AHORA FIJO
        //mqttDatos?.publishTopic(MQTT_SCAN, USER_MQTT);

        Toast.makeText(this, "Espere un momento mientras se buscan dispositivos", Toast.LENGTH_LONG).show()

        //Espera de 1 segundo
        //val handler = Handler()
        //handler.postDelayed({}, 1500)

        //mqttDatos?.suscribeTopic(MQTT_RECIBE)
        binding.scanBtnAdd.setOnClickListener {
            addTopicFunc()
        }
    }

    private fun notDo(){

    }

    private fun onMqttError(failure: MqttResultado.Failure) {
        //Toast.makeText(this, "Error connecting to mqtt ", Toast.LENGTH_SHORT).show()

    }

    private fun suscribesTopic() = runOnUiThread {
        mqttDatos?.suscribeTopic(MQTT_RECIBE)?.observe(this) { result ->
            when (result) {
                is MqttResultado.Failure -> onMqttError(result)
                is MqttResultado.Success -> onMqttMessageReceived(result)
                else -> {}
            }

        }
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

            //TODO comprobar si el formato de parseo es correcto
            Toast.makeText(this@ScanActivity, nameString, Toast.LENGTH_SHORT).show()
            if (nameString.contains("ESP")) {

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


    private fun addTopicFunc() {
        mqttDatos?.unsubscribe(MQTT_RECIBE)
        var topic: String = elementoElegio
        val dbAct = TopicDatabaseHelper(this)
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

        //Desuscribir del topico una vez se ha conseguido el nombre que se deseaba
        mqttDatos?.unsubscribe(elementoElegio)
    }


}



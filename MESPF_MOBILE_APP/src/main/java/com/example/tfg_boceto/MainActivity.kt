package com.example.tfg_boceto

import android.annotation.SuppressLint
import android.content.Intent
import android.os.Bundle
import android.util.Log

import androidx.appcompat.app.AppCompatActivity
import android.view.Menu
import android.view.MenuItem
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts.StartActivityForResult
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.tfg_boceto.adapter.EspAdapter
import com.example.tfg_boceto.databinding.ActivityMainBinding
import com.example.tfg_boceto.models.Mqtt
import com.example.tfg_boceto.models.MqttResultado
import com.example.tfg_boceto.models.MqttService
import com.example.tfg_boceto.models.persistencia.TopicDatabaseHelper
import org.eclipse.paho.client.mqttv3.MqttClient
import org.json.JSONObject
import org.json.JSONTokener


// Se pone /# para mostrar todos los subtopicos de ese topico

const val MQTT_SERVER = "188.127.160.18:1883"

/**
 * PARA ALERTAR ES MESPF/SENS/TOPIC/+/+/ALERT
 */
//const val MQTT_SERVER = "broker.mqttdashboard.com"
//const val MQTT_SCAN = "MESPF/SENS/SCAN" //TOpico donde publicas tu nombre como user
const val MQTT_SENSORES = "MESPF/SENS/"
const val MQTT_EXTRA = "MESPF/SENS/+/+/+/INFO" //AÑADIR ESTO AL FINAL DEL TOPICO
var actualTopic: String = "" // inicia sin topico
var userLogin: String = "MESPF_USER"
var userPassword: String = "MESPF_USER"
var sensoresList: MutableList<String> = mutableListOf<String>()

class MainActivity : AppCompatActivity() {


    private lateinit var binding: ActivityMainBinding
    private var esp32MutableList: MutableList<Esp32> = mutableListOf<Esp32>() //la lista que vamos a modificar y que se cargara en el recycler
    private lateinit var adapter: EspAdapter
    private var mqttData: Mqtt? = null
    private var topicToConnect: String = ""


    //Servicio de notificaciones
    private val mqttServiceIntent by lazy {
        Intent(this, MqttService::class.java)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)


        initRecyclerView()
        //Inicializar database
        readFromTopicDB()

        if(!sensoresList.isEmpty()){
            mqttData = Mqtt(MQTT_SERVER)

            mqttData?.connect(userLogin,userPassword,::subscribesTopic , ::onMqttError)

            startService(mqttServiceIntent)
        }



    }
    //TODO Acciones para realizar cuando se selecciona uno de los elementos del recyvler view
    private fun onSensorSelected(){

    }
    private fun nothing(){}
    @SuppressLint("Range")
    private fun readFromTopicDB(){

        val dbAct = TopicDatabaseHelper(this)
        val db = dbAct.readableDatabase
        val tabla = "TOPIC_TABLE"


        val cursor = db.query(tabla, arrayOf("Nombre"), null,null,null,null,null)
        val curso2 = db.query(tabla, arrayOf("Alias"), null,null,null,null,null)
        if(cursor.moveToFirst()){
            val nameColumnIndex = cursor.getColumnIndexOrThrow("Nombre")
            val aliasColumnIndex = curso2.getColumnIndexOrThrow("Alias")
            //TODO AÑADIR CADA NOMBRE DE TOPICO A UN ARRAY DE STRING Y PASARSELO DIRECTO COMO UN SOLO TOPIC
            //TODO LA DE ABAJO SERIA LA FORMA
            /**
             *
             * val topic1 = "topic1"
            val topic2 = "topic2"
            val topics = arrayOf(topic1, topic2)
            val qos = intArrayOf(1, 1)

            mqttClient.subscribe(topics, qos)
             *
             *
             */
            do{
                val esp_name = cursor.getString(nameColumnIndex)
                val esp_alias = cursor.getString(aliasColumnIndex)
                //!val name = MQTT_SENSORES + esp_name +"/+/+/INFO/#"
                val name = MQTT_SENSORES + esp_name +"/+/+/"
                topicToConnect = name
                Toast.makeText(this@MainActivity, "El topico es" + name, Toast.LENGTH_SHORT).show()
                Log.d("TOPIC", "Topico añadido :"+name )
                Log.d("TOPIC", "Nombre DB  :"+esp_name )
                Log.d("TOPIC", "Alias DB  :"+esp_alias )
                if(name != ""){

                    val newEsp = Esp32(esp_name,esp_alias, -1000.0, -1000.0, -1000.0, -1000.0, -1000.0);
                    //SI NO EXISTE ESE NOMBRE de Esp en la lista se añade y lo añadimos a elementos para suscribirnos
                    if(!esp32MutableList.any{(it.nombre_esp == esp_name)}){
                        esp32MutableList.add(newEsp)
                        adapter.notifyItemInserted(adapter.itemCount - 1) // notificamos al adapter que se añadio un item en la posicion final
                        sensoresList.add(name)
                    }

                }

                    //mqttData?.connect(userLogin,userPassword,::subscribesTopic , ::onMqttError)


                //val indexSlah =topicToConnect.lastIndexOf("/")
                //val textAfterSlash = if(indexSlah != -1) topicToConnect.substring(indexSlah +1) else ""
                //if(textAfterSlash != ""){

                //}
            }while(cursor.moveToNext() && curso2.moveToNext())
        }
        cursor.close()
        curso2.close()
        db.close()
    }

    /***********************************************************************/
    //funciones mqtt
    private fun onMqttError(result: MqttResultado.Failure) {
        //TODO Tratar mensaje error en mqtt
        Toast.makeText(this, "Error mqtt connect", Toast.LENGTH_LONG).show()
    }

    private fun subscribesTopic() = runOnUiThread {
        for (topicoActual in sensoresList){
            mqttData?.suscribeTopic(topicoActual+"INFO")?.observe(this) { result ->
                when (result) {
                    is MqttResultado.Failure -> onMqttError(result)
                    is MqttResultado.Success -> onMqttMessageReceived(result)
                    //TODO mostrar barra de carga de suscripcion a topic
                    else -> {}
                }
            }
        }

    }

    private fun onMqttMessageReceived(result: MqttResultado.Success) {
        result.payload?.run {
            //primero se recibe el nombre del topico y luego el contenido
            //por tanto hay que ver si tenemos nombre o contenido
            val msg = String(this)
            val topicCurr = mqttData!!.getTopic()

            if(topicCurr.contains("DHT22")  ||topicCurr.contains("MQ2")
                || topicCurr.contains("HC-RS04") ||topicCurr.contains("SO-SEN") ){
                //asignamos topico actual
                actualTopic = msg


                val input = topicCurr
                val parts = input.split("/")
                val desiredSubstring = parts[2]


                //TODO SUMARLE /1/INFO al final del topico
                Log.d("onMqttMessageReceived", "Topico actual :"+topicCurr)
                Log.d("onMqttMessageReceived", "Nombre en topico suscrito es :"+desiredSubstring)
                val objeto = esp32MutableList.first{ it.nombre_esp == desiredSubstring }
                val indexDht = esp32MutableList.indexOf(objeto)

                if(topicCurr.contains("DHT22")){

                    //Buscamos el nombre del sensor dentro del topic

                    objeto.temperatura = parseJSON(msg,2)
                    objeto.humedad = parseJSON(msg, 1)
                    Log.d("onMqttMessageReceived", "DHT22 valores: Temperatura: "+ objeto.temperatura +" Humedad"+objeto.humedad)
                    //TEMPERATURA Y HUMEDAD AMBIENTE
                }
                else if(topicCurr.contains("MQ2")){
                    //CONCENTRACION PARTICULAS GAS
                    objeto.concentracion_gas = parseJSON(msg, 1)
                    Log.d("onMqttMessageReceived", "MQ2 valores: Gas: "+ objeto.concentracion_gas)
                }
                else if(topicCurr.contains("HC-RS04")){
                    //NIVEL DE AGUA
                    objeto.nivel_agua = parseJSON(msg, 1)
                    Log.d("onMqttMessageReceived", "HC-RS04 valores: nivel agua: "+ objeto.nivel_agua)

                }else if(topicCurr.contains("SO-SEN")){
                    //NIVEL HUMEDAD TIERRA
                    objeto.humedad_tierra = parseJSON(msg, 1)
                    Log.d("onMqttMessageReceived", "SO-SEN valores: humedad tierra: "+ objeto.humedad_tierra)
                }
                else{
                    //No hacemos nada no es topico conocido
                    Log.d("onMqttMessageReceived", "Mensaje no determinado" + msg)
                }

                //Insertar el objeto con los atributos cambiados en la misma posicion
                esp32MutableList[indexDht] = objeto
                //Notificar la informacion ha cambiado en esa posicion
                adapter.notifyItemChanged(indexDht)

            }


            //TODO parsear cada elemento para el formato correspondiente usando las clases del paquete sensores
            else{

            }

            //val newEsp = Esp32(actualTopic, 22.0, 12.0, 1.0, true, true, false);
            //esp32MutableList.add(newEsp)
            //adapter.notifyItemInserted(adapter.itemCount - 1) // notificamos al adapter que se añadio un item en la posicion final

        }

    }

    private fun parseJSON(cadena: String, clave: Int): Double {

        val jsonObject = JSONTokener(cadena).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")

        return dt.getDouble(clave.toString())
    }

    //Recycler View
    private fun initRecyclerView() {
        //Grid layout para varios items por fila
        // le pasamos nuestra lista mutable al adapter
        adapter = EspAdapter(espList = esp32MutableList, onClickListener = { esp32 ->
            onItemSelected(esp32)
        },
            onclickEdit = { position -> onEditedItem(position) })
        val manager = LinearLayoutManager(this)
        val decoration = DividerItemDecoration(this, manager.orientation)

        binding.RecyclerESP.layoutManager = manager
        binding.RecyclerESP.adapter = adapter
        binding.RecyclerESP.addItemDecoration(decoration)
    }

    private fun onItemSelected(esp32: Esp32) {
        Toast.makeText(this, esp32.nombre_esp, Toast.LENGTH_SHORT).show()
    }

    private fun onEditedItem(position: Int) {
        //TODO pantalla para edicion de los dispositivos
        Toast.makeText(this, "Elegiste la posicion" + position, Toast.LENGTH_SHORT).show()
    }

    //FUNCIONES DE LOS BOTONES DEL MENU SUPERIOR
    private fun creaEsp() {
        val intent = Intent(this, AddEspActivity::class.java)
        resultLauncher.launch(intent)

    }

    //TODO Pasar
    private fun scanEsp(){
        val intent = Intent(this, ScanActivity::class.java)
        intent.putExtra("username", userLogin)
        intent.putExtra("password", userPassword)
        resultLauncher.launch(intent)
    }

    private fun editaEsp() {}

    //resultado de la actividad AddEspActivity
    private var resultLauncher = registerForActivityResult(StartActivityForResult()) { result ->
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        // Inflate the menu; this adds items to the action bar if it is present.
        binding.toolbar.inflateMenu(R.menu.menu_main)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {

        when (item.itemId) {
            R.id.nav_add -> scanEsp()//creaEsp()
            R.id.nav_edit -> editaEsp()
        }
        return super.onOptionsItemSelected(item)

    }





}

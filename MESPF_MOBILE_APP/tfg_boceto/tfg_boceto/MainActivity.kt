package com.example.tfg_boceto

import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts.StartActivityForResult
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.tfg_boceto.adapter.EspAdapter
import com.example.tfg_boceto.databinding.ActivityMainBinding
import com.example.tfg_boceto.models.Mqtt
import com.example.tfg_boceto.models.MqttResultado
import com.example.tfg_boceto.models.MqttService
import com.example.tfg_boceto.models.persistencia.TopicDatabaseHelper
import com.example.tfg_boceto.models.viewmodel.InfoDialog
import com.example.tfg_boceto.models.viewmodel.StatusDialog
import kotlinx.android.synthetic.main.activity_loginmqtt.*
import org.json.JSONException
import org.json.JSONObject
import org.json.JSONTokener
import java.io.*


// Se pone /# para mostrar todos los subtopicos de ese topico

//val MQTT_SERVER = "188.127.169.50:1833"
var MQTT_SERVER: String = ""
/**
 * PARA ALERTAR ES MESPF/SENS/TOPIC/+/+/ALERT
 */

//const val MQTT_SCAN = "MESPF/SENS/SCAN" //TOpico donde publicas tu nombre como user
const val MQTT_SENSORES = "MESPF/SENS/"
const val MQTT_EXTRA = "MESPF/SENS/+/+/+/INFO" //AÑADIR ESTO AL FINAL DEL TOPICO
var actualTopic: String = "" // inicia sin topico
var userLogin: String = "MESPF_USER"
var userPassword: String = "MESPF_USER"
var userName:String = "default"
var sensoresList: MutableList<String> = mutableListOf<String>()
var idMensaje: Int = 1

var editActivityFinish: Boolean = false
//TODO terminar editar esp pantalla para cambiar el nombre
//TODO hacer pantalla de umbrales mandando float
//TODO cambiar color de bolita cuando ha pasado tiempo sin recibir mensajes de un dispositivo
//TODO desuscribir

//TODO hacer comando set y getgpios

/*************************************************/
//PUBLICA EN TOPICO EJEMPLO
//MESPF/SENS/ESP32_f867BC/MQ2/1/INFO
/*************************************************/
/*
Ejemplo de STATUS:
{
  "DT": {
    "1": "192.168.68.78",
    "2": "ESP32_f867BC",
    "3": "21:23:01",
    "4": "003 00:19"
  },
  "TS": "21:23:01 29/12/2022"
}
 */
var mqttData: Mqtt? = null
class MainActivity : AppCompatActivity() {


    private lateinit var binding: ActivityMainBinding
    private var esp32MutableList: MutableList<Esp32> = mutableListOf<Esp32>() //la lista que vamos a modificar y que se cargara en el recycler
    private lateinit var adapter: EspAdapter

    private var topicToConnect: String = ""
    private var newLoginFlag: Boolean = false
    private var succesLogin: Boolean = false
    private lateinit var selectedEsp: Esp32
    private var posicionSelected: Int = -1
    private var otraActivitdad: Boolean = false


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)
        var newLogBtn: Boolean = false
        var existLogBtn: Boolean = false
        posicionSelected = -1
        //Mostramos boton para elegir login o no

        val extras = intent.extras
        if(extras != null){
            otraActivitdad = extras.getBoolean("vuelta")
        }

        if(otraActivitdad){
            binding.layoutLoginMain.visibility = View.GONE
            binding.layoutPreLoginMain.visibility = View.GONE
            binding.layoutRecycler.visibility = View.VISIBLE
            leerDbYConnectMqtt()

            return
        }
        binding.layoutPreLoginMain.visibility = View.VISIBLE

        binding.newAccountBtn.setOnClickListener{
            newLogBtn = true;
            binding.layoutPreLoginMain.visibility = View.GONE
            binding.layoutLoginMain.visibility = View.VISIBLE
        }
        binding.existAccountBtn.setOnClickListener{
            existLogBtn = true;
            binding.layoutPreLoginMain.visibility = View.GONE
            if(checkLoginFile()){

                binding.layoutLoginMain.visibility = View.GONE
                binding.layoutPreLoginMain.visibility = View.GONE
                binding.layoutRecycler.visibility = View.VISIBLE
                leerDbYConnectMqtt()

            }
            else{
                Toast.makeText(this@MainActivity, "No hay datos de usuario guardados", Toast.LENGTH_LONG).show()
                binding.layoutPreLoginMain.visibility = View.VISIBLE
            }
        }

        binding.login.setOnClickListener{
            if(binding.ipMqtt.text!!.isEmpty() || binding.usernameMqtt.text!!.isEmpty() ||
                    binding.passwordMqtt.text!!.isEmpty() || binding.usernameNomqtt.text!!.isEmpty()){
                Toast.makeText(this@MainActivity, "Debes rellenar todos los campos", Toast.LENGTH_SHORT).show()

            }
            else{
                MQTT_SERVER= binding.ipMqtt.text.toString()
                userLogin = binding.usernameMqtt.text.toString()
                userPassword = binding.passwordMqtt.text.toString()
                userName = binding.usernameNomqtt.text.toString()

                Log.d("Mqtt", "IP es "+ MQTT_SERVER+"user $userLogin, pass $userPassword, Nombre: $userName")

                val sharedPref = getSharedPreferences("myPrefs", Context.MODE_PRIVATE)
                val server = sharedPref.edit()
                server.putString("MQTT_SERVER", MQTT_SERVER)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
                    server.apply()
                }


                binding.layoutLoginMain.visibility = View.GONE
                binding.layoutPreLoginMain.visibility = View.GONE
                binding.layoutRecycler.visibility = View.VISIBLE
                newLoginFlag = true
                //TODO falta guardar en fichero una vez se ha insertado usuario y se
                //TODO logueado correctamente
                //TODO hacerlo con flag que compruebe que es la primera vez
                //TODO que se inserta ese usuario para guardarlo en el fichero

                val ip =  binding.ipMqtt.text.toString()
                val user = binding.usernameMqtt.text.toString()
                val pass = binding.passwordMqtt.text.toString()
                val userNoMqtt = binding.usernameNomqtt.text.toString()
                // Get an instance of SharedPreferences
                val sharedPreferences = getSharedPreferences("mqtt_prefs", Context.MODE_PRIVATE)

                // Save the values
                val editor = sharedPreferences.edit()
                editor.putString("ip", ip)
                editor.putString("nameMqtt", user)
                editor.putString("passMqtt", pass)
                editor.putString("nameNormal", userNoMqtt)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
                    editor.apply()
                }

                leerDbYConnectMqtt()
            }




        }
        /**
        binding.saveUserBtn.setOnClickListener{

            if(binding.ipMqtt.text!!.isEmpty() || binding.usernameMqtt.text!!.isEmpty() ||
                binding.passwordMqtt.text!!.isEmpty() || binding.usernameNomqtt.text!!.isEmpty()){
                Toast.makeText(this@MainActivity, "Debes rellenar todos los campos", Toast.LENGTH_SHORT).show()

            }
            else {

                val ip =  binding.ipMqtt.text.toString()
                val user = binding.usernameMqtt.text.toString()
                val pass = binding.passwordMqtt.text.toString()
                val userNoMqtt = binding.usernameNomqtt.text.toString()
                // Get an instance of SharedPreferences
                val sharedPreferences = getSharedPreferences("mqtt_prefs", Context.MODE_PRIVATE)

                // Save the values
                val editor = sharedPreferences.edit()
                editor.putString("ip", ip)
                editor.putString("nameMqtt", user)
                editor.putString("passMqtt", pass)
                editor.putString("nameNormal", userNoMqtt)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
                    editor.apply()
                }


                Log.d("MainActivity", "Datos guardados IP: $ip, Usuario: $user, Contraseña: $pass, Usuario No Mqtt: $userNoMqtt")
            }
        }
        */


    }
    //TODO Acciones para realizar cuando se selecciona uno de los elementos del recyvler view
    private fun onSensorSelected(){

    }
    //Servicio de notificaciones
    private val mqttServiceIntent by lazy {
        val intent = Intent(this, MqttService::class.java)
        intent.putExtra("SERVER", MQTT_SERVER)
    }


    private fun leerDbYConnectMqtt(){
        initRecyclerView()
        //Inicializar database
        readFromTopicDB()

        if(sensoresList.isNotEmpty()){
            mqttData = Mqtt(MQTT_SERVER)

            mqttData?.connect(userLogin,userPassword,::subscribesTopic , ::onMqttError)
            //No iniciar servicio si no es la primera vez que se abre la aplicacion

            /**LANZAR SERVICIO*/
            if(!otraActivitdad){
                startService(mqttServiceIntent)
                /**
                //Convertimos la lista de sensores a json
                val jsonSensoresList = Gson().toJson(sensoresList)
                val dataWorker = Data.Builder()
                    .putString("listSensores", jsonSensoresList)
                    .putString("ip", MQTT_SERVER)
                    .putString("userMqtt", userLogin)
                    .putString("passMqtt", userPassword)
                    .putString("name", userName)
                    .build()

                val workLaunch = OneTimeWorkRequestBuilder<MqttWorker>()
                    .setInputData(dataWorker)
                    .build()

                WorkManager.getInstance(this).enqueue(workLaunch)
           */
            }
                //!startService(mqttServiceIntent)
        }
    }

    private fun checkLoginFile(): Boolean{
        //val filename = "login_mqtt.txt"
        //val file = File(this@MainActivity.filesDir, filename)

        // Get an instance of SharedPreferences
        val sharedPreferences = getSharedPreferences("mqtt_prefs", Context.MODE_PRIVATE)

        //if (sharedPreferences.contains("ip")) {
            // Read the values

            val ip = sharedPreferences.getString("ip", null)
            val nameMqtt = sharedPreferences.getString("nameMqtt", null)
            val passMqtt = sharedPreferences.getString("passMqtt", null)
            val nameNormal = sharedPreferences.getString("nameNormal", null)
            MQTT_SERVER = ip.toString()
            userLogin = nameMqtt.toString()
            userPassword = passMqtt.toString()
            userName = nameNormal.toString()

            Log.d("MainActivity", "IP: $ip, Usuario: $nameMqtt, Contraseña: $passMqtt, Usuario No Mqtt: $nameNormal")
        if(ip != null)
            return true
        else {
            Log.d("MainActivity", "No hay informacion de login mqtt")
            return false
        }
        return false
    }
    private fun nothing(){}
    @SuppressLint("Range")
    private fun readFromTopicDB(){

        val dbAct = TopicDatabaseHelper(this)
        val db = dbAct.readableDatabase
        val tabla = "TOPIC_TABLE"


        val cursor = db.query(tabla, arrayOf("Nombre"), null,null,null,null,null)
        //val curso2 = db.query(tabla, arrayOf("Alias"), null,null,null,null,null)
        if(cursor.moveToFirst()){
            val nameColumnIndex = cursor.getColumnIndexOrThrow("Nombre")
            //val aliasColumnIndex = curso2.getColumnIndexOrThrow("Alias")

            do{
                val esp_name = cursor.getString(nameColumnIndex)
                val esp_alias = dbAct.getAliasDelNombre(esp_name)
                //!val name = MQTT_SENSORES + esp_name +"/+/+/INFO/#"
                val name = MQTT_SENSORES + esp_name +"/+/+/"
                topicToConnect = name
                Log.d("TOPIC", "Topico añadido :"+name )
                Log.d("TOPIC", "Nombre DB  :"+esp_name )
                Log.d("TOPIC", "Alias DB  :"+esp_alias )
                if(name != ""){
                    var temperat: Double = -1000.0
                    var humidity: Double = -1000.0
                    var wat_levl: Double = -1000.0
                    var gas_c: Double = -1000.0
                    var hum_ter: Double = -1000.0
                    val newEsp = Esp32(esp_name,esp_alias,1, temperat, humidity, wat_levl, gas_c, hum_ter, "");
                    //SI NO EXISTE ESE NOMBRE de Esp en la lista se añade y lo añadimos a elementos para suscribirnos
                    if(!esp32MutableList.any{(it.nombre_esp == esp_name)}){
                        esp32MutableList.add(newEsp)
                        adapter.notifyItemInserted(adapter.itemCount - 1) // notificamos al adapter que se añadio un item en la posicion final
                        sensoresList.add(name)
                    }

                }
            }while(cursor.moveToNext())
        }
        cursor.close()
        db.close()
    }

    /***********************************************************************/
    //funciones mqtt
    private fun onMqttError(result: MqttResultado.Failure) {
        //TODO Tratar mensaje error en mqtt
        Log.d("Mqtt", "Error connect mqtt")
        binding.layoutPreLoginMain.visibility = View.GONE
        binding.layoutRecycler.visibility = View.GONE
        binding.layoutLoginMain.visibility = View.VISIBLE


    }

    private fun subscribesTopic() = runOnUiThread {

        for (topicoActual in sensoresList){
            succesLogin = true
            mqttData?.suscribeTopic(topicoActual+"INFO")?.observe(this) { result ->
                when (result) {
                    is MqttResultado.Failure -> onMqttError(result)
                    is MqttResultado.Success -> onMqttMessageReceived(result)
                    //TODO mostrar barra de carga de suscripcion a topic
                    else -> {}
                }
            }
        }
        //Nos suscribimos a todos los temas que seran todos los sensores
        mqttData?.suscribeTopic("MESPF/USR/$userName/REFRESH/+/+/+/INFO")?.observe(this){result->
            when (result) {
                is MqttResultado.Failure -> onMqttError(result)
                is MqttResultado.Success -> parseRefreshMsgresult(result)
                //TODO mostrar barra de carga de suscripcion a topic
                else -> {}
            }
        }

        for (t in sensoresList){
            val input = t
            val parts = input.split("/")
            val desiredSubstring = parts[2]

            val tpic = "MESPF/SENS/$desiredSubstring/STATUS/1/CMD/REFRESH"
            Log.d("ResfreshMsg", "Publicamos en el topico: $tpic")

            //mqttData?.publishTopic(tpic, userName)
            //suscribir a status de refresh
            mqttData?.suscribeTopic("MESPF/USR/$userName/REFRESH/$desiredSubstring/STATUS/1/INFO")?.observe(this){
                result->
                when(result){
                    is MqttResultado.Failure -> onMqttError(result)
                    is MqttResultado.Success -> onMqttMessageReceived(result)

                    else -> {}
                }
            }
            // PARA MANDAR JSON
            mqttData?.publishTopic(tpic, "{\"USER\":\"$userName\",\"ID\":$idMensaje}")
            ++idMensaje
        }




    }

    /***************************PARSEO MENSAJES DEL REFRESH******************************/
    private fun parseRefreshMsgresult(result: MqttResultado.Success) {
        result.payload?.run {
            //primero se recibe el nombre del topico y luego el contenido
            //por tanto hay que ver si tenemos nombre o contenido
            val msg = String(this)
            val topicCurr = mqttData!!.getTopic()
            if(!msg.contains("DT"))
                return
            Log.d("RefreshMsg", "Mensaje recibido $msg")
            Log.d("RefreshMsg", "Topico $topicCurr")
            if(topicCurr.contains("MESPF/USR/$userName/REFRESH") && ((topicCurr.contains("DHT22") || topicCurr.contains("MQ2")
                || topicCurr.contains("HC-RS04") ||topicCurr.contains("SO-SEN"))) && !topicCurr.contains("STATUS")){
                //asignamos topico actual
                actualTopic = topicCurr


                val input = topicCurr
                val parts = input.split("/")
                val desiredSubstring = parts[4]
                val numero = parts[6].toInt()


                //TODO SUMARLE /1/INFO al final del topico
                Log.d("parseRefreshMsgresult", "Topico actual :"+topicCurr)
                Log.d("parseRefreshMsgresult", "Nombre en topico suscrito es :"+desiredSubstring)
                var yaExiste: Boolean = true
                var indexDht: Int = esp32MutableList.size-1
                var objeto: Esp32 = Esp32(desiredSubstring, desiredSubstring+" ("+numero+")", numero, -1000.0,-1000.0,-1000.0, -1000.0, -1000.0, "")
                try{
                    objeto = esp32MutableList.first{ it.nombre_esp == desiredSubstring && it.numero == numero}

                }catch (e:NoSuchElementException){
                    yaExiste = false
                }

                if(yaExiste)
                    indexDht = esp32MutableList.indexOf(objeto)
                else{
                    //Coincide nombre pero no numero
                    if(esp32MutableList.first{it.nombre_esp == desiredSubstring} != null){

                        objeto = Esp32(desiredSubstring, esp32MutableList.first{it.nombre_esp == desiredSubstring}.alias_esp+" ("+numero+")", numero, -1000.0, -1000.0, -1000.0, -1000.0, -1000.0, "")
                        Log.d("Sensores", "Se añadio el sensor $objeto")
                    }
                    else
                        return
                }

                Log.d("parseRefreshMsgresult", "Mensaje recibido $msg")
                if(topicCurr.contains("DHT22")){

                    //Buscamos el nombre del sensor dentro del topic

                    objeto.temperatura = kotlin.math.round( parseJSON(msg,2) * 100) / 100.0


                    objeto.humedad = kotlin.math.round( parseJSON(msg,1) * 100) / 100.0
                    Log.d("onRefreshMessageReceived", "DHT22 valores: Temperatura: "+ objeto.temperatura +" Humedad:"+objeto.humedad)
                    //TEMPERATURA Y HUMEDAD AMBIENTE
                }
                else if(topicCurr.contains("MQ2")){
                    //CONCENTRACION PARTICULAS GAS
                    objeto.concentracion_gas = kotlin.math.round( parseJSON(msg,1) * 100) / 100.0
                    Log.d("onRefreshMessageReceived", "MQ2 valores: Gas: "+ objeto.concentracion_gas)
                }
                else if(topicCurr.contains("HC-RS04")){
                    //NIVEL DE AGUA
                    objeto.nivel_agua = kotlin.math.round( parseJSON(msg,1) * 100) / 100.0
                    Log.d("onRefreshMessageReceived", "HC-RS04 valores: nivel agua: "+ objeto.nivel_agua)

                }else if(topicCurr.contains("SO-SEN")){
                    //NIVEL HUMEDAD TIERRA
                    objeto.humedad_tierra = kotlin.math.round( parseJSON(msg,1) * 100) / 100.0
                    Log.d("onRefreshMessageReceived", "SO-SEN valores: humedad tierra: "+ objeto.humedad_tierra)
                }
                else{
                    //No hacemos nada no es topico conocido
                    Log.d("onRefreshMessageReceived", "Mensaje no determinado" + msg)
                }

                if(yaExiste){
                    //Insertar el objeto con los atributos cambiados en la misma posicion
                    Log.d("ResfreshMsg", "Modificamos el objeto ${objeto.toString()} en la posicion ${adapter.itemCount-1}")
                    esp32MutableList[indexDht] = objeto
                    adapter.notifyItemChanged(indexDht)
                    Log.d("ListaModifica", "La lista al modificar el elemento ${objeto.alias_esp} es: ${esp32MutableList.toString()}")
                    //Notificar la informacion ha cambiado en esa posicion
                }
                else{
                    Log.d("ResfreshMsg", "Añadimos el objeto ${objeto.toString()} en la posicion ${adapter.itemCount-1}")
                    esp32MutableList.add(objeto)
                    adapter.notifyItemInserted(adapter.itemCount - 1)
                    Log.d("ListaModifica", "La lista despues de añadir ${objeto.alias_esp}  es ${esp32MutableList.toString()}")
                }


            }
            }
    }
    private fun unsuscribeTopic(topic: String) = runOnUiThread{
        mqttData?.unsubscribe(topic)
    }
    private fun borrarTopicDB(topicoUnsub: String){
        val dbAct = TopicDatabaseHelper(this)
        val db = dbAct.writableDatabase
        val tabla = "TOPIC_TABLE"
        val sql = "DELETE FROM TOPIC_TABLE WHERE Nombre = ?"

        val args = arrayOf(topicoUnsub.toString())
        val numRowsDelete = db.execSQL(sql, args)

        db.close()

        Log.d("MQTT", "Eliminados $numRowsDelete filas")

    }
    private fun onMqttMessageReceived(result: MqttResultado.Success) {
        result.payload?.run {
            //primero se recibe el nombre del topico y luego el contenido
            //por tanto hay que ver si tenemos nombre o contenido
            val msg = String(this)

            val topicCurr = mqttData!!.getTopic()

            if(!msg.contains("DT"))
                return


            actualTopic = topicCurr

            //PARA MENSAJE DE STATUS
            if(topicCurr.contains("/STATUS/1/INFO") && topicCurr.contains("REFRESH")){
                //var hora: String = ""
                //hora = parseHora(msg)

                val input = topicCurr
                val parts = input.split("/")
                val desiredSubstring = parts[4]

                Log.d("STATUS", "Dispositivo status: $desiredSubstring")
                Log.d("STATUS", "Mensaje status: $msg")
                var objeto: Esp32 = esp32MutableList.first{ it.nombre_esp == desiredSubstring}

                val posicionEsp = esp32MutableList.indexOf(objeto)

                objeto.last_publish = msg

                Log.d("ResfreshMsg", "Modificamos el objeto ${objeto.toString()} en la posicion ${adapter.itemCount-1}")
                esp32MutableList[posicionEsp] = objeto
                adapter.notifyItemChanged(posicionEsp)

                return
            }

            //proteccion para no parsear mensajes del refresh
            if(!topicCurr.contains("MESPF/SENS") || !topicCurr.contains("INFO") ||topicCurr.contains("REFRESH"))
                return


            if(topicCurr.contains("DHT22")  ||topicCurr.contains("MQ2")
                || topicCurr.contains("HC-RS04") ||topicCurr.contains("SO-SEN") && !topicCurr.contains("STATUS")){
                //asignamos topico actual


                val input = topicCurr
                val parts = input.split("/")
                val desiredSubstring = parts[2]
                val numero = parts[4].toInt()


                //TODO SUMARLE /1/INFO al final del topico
                Log.d("onMqttMessageReceived", "Topico actual :"+topicCurr)
                Log.d("onMqttMessageReceived", "Nombre en topico suscrito es :"+desiredSubstring)


                var yaExiste: Boolean = true
                var indexDht: Int = esp32MutableList.size-1
                var objeto: Esp32 = Esp32(desiredSubstring, desiredSubstring+" ("+numero+")", numero, -1000.0,-1000.0,-1000.0, -1000.0, -1000.0, "")
                try{
                    objeto = esp32MutableList.first{ it.nombre_esp == desiredSubstring && it.numero == numero}

                }catch (e:NoSuchElementException){
                    yaExiste = false
                }



                if(yaExiste)
                    indexDht = esp32MutableList.indexOf(objeto)
                else{
                    //Coincide nombre pero no numero
                    if(esp32MutableList.first{it.nombre_esp == desiredSubstring} != null){

                        objeto = Esp32(desiredSubstring, esp32MutableList.first{it.nombre_esp == desiredSubstring}.alias_esp+" ("+numero+")", numero, -1000.0, -1000.0, -1000.0, -1000.0, -1000.0, "")
                        Log.d("Sensores", "Se añadio el sensor $objeto")
                    }
                    else
                        return
                }

                //val objeto = esp32MutableList.first{ it.nombre_esp == desiredSubstring }
                //val indexDht = esp32MutableList.indexOf(objeto)




                if(topicCurr.contains("DHT22")){

                    //Buscamos el nombre del sensor dentro del topic

                    objeto.temperatura = kotlin.math.round( parseJSON(msg,2) * 100) / 100.0


                    objeto.humedad = kotlin.math.round( parseJSON(msg,1) * 100) / 100.0
                    Log.d("onMqttMessageReceived", "DHT22 valores: Temperatura: "+ objeto.temperatura +" Humedad"+objeto.humedad)
                    //TEMPERATURA Y HUMEDAD AMBIENTE
                }
                else if(topicCurr.contains("MQ2")){
                    //CONCENTRACION PARTICULAS GAS
                    objeto.concentracion_gas = kotlin.math.round( parseJSON(msg,1) * 100) / 100.0
                    Log.d("onMqttMessageReceived", "MQ2 valores: Gas: "+ objeto.concentracion_gas)
                }
                else if(topicCurr.contains("HC-RS04")){
                    //NIVEL DE AGUA
                    objeto.nivel_agua = kotlin.math.round( parseJSON(msg,1) * 100) / 100.0
                    Log.d("onMqttMessageReceived", "HC-RS04 valores: nivel agua: "+ objeto.nivel_agua)

                }else if(topicCurr.contains("SO-SEN")){
                    //NIVEL HUMEDAD TIERRA
                    objeto.humedad_tierra = kotlin.math.round( parseJSON(msg,1) * 100) / 100.0
                    Log.d("onMqttMessageReceived", "SO-SEN valores: humedad tierra: "+ objeto.humedad_tierra)
                }
                else{
                    //No hacemos nada no es topico conocido
                    Log.d("onMqttMessageReceived", "Mensaje no determinado" + msg)
                }


                if(yaExiste){
                    //Insertar el objeto con los atributos cambiados en la misma posicion
                    Log.d("ResfreshMsg", "Modificamos el objeto ${objeto.toString()} en la posicion ${adapter.itemCount-1}")
                    esp32MutableList[indexDht] = objeto
                    adapter.notifyItemChanged(indexDht)
                    Log.d("ListaModifica", "La lista al modificar el elemento ${objeto.alias_esp} es: ${esp32MutableList.toString()}")
                    //Notificar la informacion ha cambiado en esa posicion
                }
                else{
                    Log.d("ResfreshMsg", "Añadimos el objeto ${objeto.toString()} en la posicion ${adapter.itemCount-1}")
                    esp32MutableList.add(objeto)
                    adapter.notifyItemInserted(adapter.itemCount - 1)
                    Log.d("ListaModifica", "La lista despues de añadir ${objeto.alias_esp}  es ${esp32MutableList.toString()}")
                }

                //Insertar el objeto con los atributos cambiados en la misma posicion
                //esp32MutableList[indexDht] = objeto
                //Notificar la informacion ha cambiado en esa posicion
                //adapter.notifyItemChanged(indexDht)

            }
            //val newEsp = Esp32(actualTopic, 22.0, 12.0, 1.0, true, true, false);
            //esp32MutableList.add(newEsp)
            //adapter.notifyItemInserted(adapter.itemCount - 1) // notificamos al adapter que se añadio un item en la posicion final

        }

    }
    private fun parseHora(cadena: String): String{
        val jsonHora = JSONTokener(cadena).nextValue() as JSONObject
        var tiempo: String = ""
        try{
            tiempo = jsonHora.getString("ST")
        }catch(e:JSONException){Log.d("JSON", "Excepcion parseando hora")}

        return tiempo
    }

    private fun parseJSON(cadena: String, clave: Int): Double {

        val jsonObject = JSONTokener(cadena).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")

        var valor: Double = 0.0
        try{
            valor = dt.getDouble(clave.toString())
        }catch (e:JSONException){}

        return valor
    }

    //Recycler View
    private fun initRecyclerView() {
        //Grid layout para varios items por fila
        // le pasamos nuestra lista mutable al adapter
        adapter = EspAdapter(espList = esp32MutableList,
            onClickListener = { posicion ->
            onItemSelected(posicion) },
            onClickRefresh = { position -> onRefreshItem(position)},
            onclickStatus = {position -> onStatusClickedItem(position)})
        val manager = LinearLayoutManager(this)
        val decoration = DividerItemDecoration(this, manager.orientation)

        binding.RecyclerESP.layoutManager = manager
        binding.RecyclerESP.adapter = adapter
        binding.RecyclerESP.addItemDecoration(decoration)
    }


    //ON status button pulsado
    private fun onItemSelected(posicion: Int) {
        //selectedEsp = esp32
        posicionSelected = posicion
        Log.d("Adapter", "Seleccionada la posicion en onItemSelected $posicion")

        //Toast.makeText(this, esp32.nombre_esp, Toast.LENGTH_SHORT).show()
    }


    private fun onRefreshItem(position: Int) {
        if(esp32MutableList[position].numero > 1)
            return
        val disp = esp32MutableList[position].nombre_esp
        val tpic = "MESPF/SENS/$disp/STATUS/1/CMD/REFRESH"
        //mqttData?.publishTopic(tpic, userName)
        mqttData?.publishTopic(tpic, "{\"USER\":\"$userName\",\"ID\":$idMensaje}")
    //Toast.makeText(this, "Elegiste la posicion" + position, Toast.LENGTH_SHORT).show()
    }

    private fun onStatusClickedItem(position: Int){
        //TODO mostrar aqui ventana emergente con el status
        if(esp32MutableList[position].numero > 1)
            return
        Log.d("STATUS", "Enviamos al dialog: ${esp32MutableList[position].last_publish}")
        val dialogoStatus = StatusDialog(this,esp32MutableList[position].last_publish)

        dialogoStatus.show()
    }


    private fun infoMain() {
        val dialogoInfo = InfoDialog(this)

        dialogoInfo.show()
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
        finish()
    }

    private fun editaEsp() {

        if(posicionSelected == -1){
            Toast.makeText(this@MainActivity, "Debes elegir un elemento", Toast.LENGTH_SHORT).show()
            return
        }

        if(esp32MutableList[posicionSelected].numero > 1)
            return

        val intent = Intent(this, EditEspActivity::class.java)
        intent.putExtra("nombre_esp", esp32MutableList[posicionSelected].nombre_esp)
        intent.putExtra("alias_esp", esp32MutableList[posicionSelected].alias_esp)
        intent.putExtra("username", userLogin)
        intent.putExtra("password", userPassword)
        resultLauncher.launch(intent)

    }

    //resultado de la actividad AddEspActivity
    private var resultLauncher = registerForActivityResult(StartActivityForResult()) { result ->
    }

    private fun desuscribeEsp(){
        if(posicionSelected == -1){
            Toast.makeText(this, "No has elegido ningun elemento para eliminar", Toast.LENGTH_SHORT).show()
        }
        else{
            val auxEsp = esp32MutableList[posicionSelected]
            val nombreDisp = auxEsp.nombre_esp
            esp32MutableList.removeAt(posicionSelected)
            adapter.notifyItemRemoved(posicionSelected)

            //Borramos todas las ocurrencias de este disposiitov
            esp32MutableList.removeAll{esp -> esp.nombre_esp == nombreDisp}
            adapter.notifyDataSetChanged()
            Log.d("MQTT", "Elemento quitado de la lista: ${auxEsp.nombre_esp}")
            borrarTopicDB(auxEsp.nombre_esp)
            Log.d("MQTT", "Elemento borrado de db")
      
            val auxName = MQTT_SENSORES + auxEsp.nombre_esp +"/+/+/"
            Log.d("MQTT", "Elemento desuscrito $auxName")
            unsuscribeTopic(auxName)

            //Volvemos a poner la posicion -1
            posicionSelected = -1


        }
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        // Inflate the menu; this adds items to the action bar if it is present.
        binding.toolbar.inflateMenu(R.menu.menu_main)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {

        when (item.itemId) {
            R.id.nav_add -> scanEsp()
            R.id.nav_eliminar -> desuscribeEsp() //eliminar
            R.id.nav_editar -> editaEsp()
            R.id.infoButton -> infoMain()
        }
        return super.onOptionsItemSelected(item)

    }



    override fun onResume() {
        super.onResume()

        if(editActivityFinish){
            editActivityFinish = false

            sensoresList.clear()
            adapter.notifyDataSetChanged()
            readFromTopicDB()

        }
    }

    override fun onBackPressed() {
        super.onBackPressed()

        mqttData?.disconnect()
    }


    override fun onDestroy() {
        super.onDestroy()

    }





}

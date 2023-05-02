package com.example.tfg_boceto

import android.annotation.SuppressLint
import android.content.ContentValues
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
import android.util.Log
import android.view.View
import android.view.inputmethod.EditorInfo
import android.widget.*
import androidx.core.widget.addTextChangedListener
import com.example.tfg_boceto.databinding.ActivityEditEspBinding
import com.example.tfg_boceto.models.MqttResultado
import com.example.tfg_boceto.models.persistencia.TopicDatabaseHelper
import com.example.tfg_boceto.models.sensores.configuraciones.*
import org.json.JSONException
import org.json.JSONObject
import org.json.JSONTokener


private var userMqtt: String = ""
private var passMqtt: String = ""
private var aliasEsp: String = ""
private var nombreEsp: String = ""
private var tipoOperacion: Int = 0 // 0 alarmas, 1 gpios, 2 parametros
private var setConfObj: SetConfClass = SetConfClass(userName, 0,0,"0","0","0","0")

class EditEspActivity : AppCompatActivity() {

    //Objetos de configuracion de cada sensor
    private var dht22cfg: DhtConfig = DhtConfig(0, 0, 0, 0, 0, 0, 0)
    private var hcrs04cfg: Hcrs04Config = Hcrs04Config(0, 0, 0, 0, 0, 0, 0, 0)
    private var mq2Config: Mq2Config = Mq2Config(0, 0, 0, 0, 0)
    private var soSenConfig: SoSenConfig = SoSenConfig(0, 0, 0, 0, 0)


    private var tipoSensor: String = ""
    private var numSensor: Int = 0
    private var valorAlarma: Int = 0
    private var arg2: Float = 0.0F
    private var arg4: Float = 0.0F
    private var arg5: Float = 0.0F



    //TODO Poner recuadro para juntar los dos elementos que son sobre lo mismo para asi tenerlos bien separados
    private lateinit var binding: ActivityEditEspBinding
    @SuppressLint("ResourceType")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityEditEspBinding.inflate(layoutInflater)
        setContentView(binding.root)
        inicializaSensores()
        recibeVariablesIntent()

        binding.editNameTopic.setText(nombreEsp)

        /**Listener para cuando cambia el valor del alias*/

        binding.saveEditDatabase.setOnClickListener{
            val dbTopic = TopicDatabaseHelper(this)
            Log.d("Database", "Update alias of: $nombreEsp to ${binding.editNameTopic.text.toString()}")
            val filas= dbTopic.updateAlias(nombreEsp, binding.editNameTopic.text.toString())
            if(filas == 0)
                Toast.makeText(this@EditEspActivity, "No se ha podido actualizar", Toast.LENGTH_SHORT).show()
           else
               Toast.makeText(this@EditEspActivity, "Se ha actualizado con exito la base de datos", Toast.LENGTH_SHORT).show()
        }
        /**Spinner de las 3 opciones de operacion
        //OPERACIÓN 0: Cambio de los valores de la alarma.
        //OPERACIÓN 1: Modificación de los GPIOS
        //OPERACIÓN 2: Modificación de los parámetros(HCR profundidad tanque) */
        val operacionesOpt = arrayOf("OPERACION 0: Valores de alarma", "OPERACION 1: GPIO", "OPERACION 2: PARAMETROS EXTRA")
        val adapterOpciones = ArrayAdapter(this@EditEspActivity, R.layout.support_simple_spinner_dropdown_item, operacionesOpt)
        binding.operacionSpinner.adapter = adapterOpciones
        binding.operacionSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener{
            override fun onNothingSelected(p0: AdapterView<*>?) {
            }

            override fun onItemSelected(p0: AdapterView<*>?, p1: View?, position: Int, p3: Long) {
                tipoOperacion = position
                Log.d("EditEsp", "Operacion: $tipoOperacion")
            }
        }


        /**Eleccion numero de sensor**/
        binding.numeroSensor.maxValue = 100
        binding.numeroSensor.minValue = 1
        //Spinner de tipos de sensores
        val options = arrayOf("DHT22", "MQ2", "SO-SEN", "HC-RS04")
        val adapter = ArrayAdapter(this@EditEspActivity,R.layout.support_simple_spinner_dropdown_item, options)
        binding.spinnerEsp.adapter = adapter
        binding.spinnerEsp.onItemSelectedListener = object : AdapterView.OnItemSelectedListener{
            override fun onNothingSelected(p0: AdapterView<*>?) {
            }
            override fun onItemSelected(p0: AdapterView<*>?, p1: View?, position: Int, p3: Long) {
                tipoSensor = options[position]
                if(tipoSensor == "DHT22"){
                    verDht()
                }
                else if(tipoSensor == "MQ2"){
                    verMq2()
                }

                else if(tipoSensor == "S0-SEN"){
                    verSoSen()
                }

                else if(tipoSensor == "HC-RS04"){
                    verHcr()
                }
            }
        }

        //Nos suscribimos al topico donde recibiremos los resultados de las consultas despues
        val topico = "MESPF/USR/$userName/RESP"
        Log.d("EditEsp", "Intentamos suscribir al topico $topico")
        if(tipoSensor != "DHT22" && tipoSensor != "MQ2" && tipoSensor != "SO-SEN" && tipoSensor != "HC-RS04")
            Toast.makeText(this@EditEspActivity, "Elige un parametro posible para el sensor $tipoSensor", Toast.LENGTH_SHORT).show()

        Log.d("EditEsp", "Intentamos suscribir al topico$topico")
        mqttData?.suscribeTopic(topico)?.observe(this){ result ->
            when(result){
                is MqttResultado.Failure -> onMqttErrorSuscribeConsulta(result)
                is MqttResultado.Success -> onMqttMessageConsultaRec(result)

                else -> {}
            }

        //Boton para suscribirse a getconf y obtener
        binding.consultaDatosBtn.setOnClickListener{
            if(mqttData?.connected() == false)
                Log.d("EditEsp", "No Estamos conectados al servidor Mqtt")

            numSensor = binding.numeroSensor.value
            // Montamos el topico
                var topico: String =  "MESPF/SENS/"+ nombreEsp+"/"+tipoSensor+"/"+numSensor+"/CMD/GETCONF"
            Log.d("EditEsp", "publicamos al topico $topico")

            //Publicamos para poder consultar los datos
            val mensaje = "{\n\"USER\":\"$userName\",\n\"ID\":$idMensaje\n}"
            Log.d("EditEsp", "Se envia el mensaje: $mensaje")
            mqttData?.publishTopic(topico, mensaje)

            idMensaje++

        }
        //mqttData?.suscribeTopic()

    }

        /**LISTENER PARA BOTON GUARDAR**/
        binding.guardaEdicionBtn.setOnClickListener{
            guardarDatos()
        }

}



    /**Funciones para configurar los valores de que sensor se ven en pantalla*/
    private fun verDht(){
        binding.arg4EditText.setText(dht22cfg.uHumedadSuperior.toString())
        binding.arg5EditText.setText(dht22cfg.uHumedadInferior.toString())
        binding.tvArg4.text = "Umbral nivel humedad superior"
        binding.tvArg5.text = "Umbral nivel humedad inferior"
        binding.arg6EditText.setText(dht22cfg.uTempSuperior.toString())
        binding.arg7EditText.setText(dht22cfg.uTempInferior.toString())
        binding.tvArg6.text = "Umbral temperatura superior"
        binding.tvArg7.text = "Umbral temperatura inferior"

        //habilitamos esos recuadros
        binding.arg6EditText.isEnabled = true
        binding.arg7EditText.isEnabled = true

        binding.extraInfo.text = "GPIO PIN:"
        binding.gpioEditText.setText(dht22cfg.Data.toString())

        binding.gpioEditText2.visibility = View.GONE
        binding.gpio2TV.visibility = View.GONE
    }
    private fun verMq2(){
        binding.arg4EditText.setText(mq2Config.uGasSuperior.toString())
        binding.arg5EditText.setText(mq2Config.uGasInferior.toString())
        binding.tvArg4.text = "Umbral nivel concentracion gas superior"
        binding.tvArg5.text = "Umbral nivel concentracion gas inferior"
        binding.tvArg6.text = "No hay mas parametros"

        binding.arg6EditText.setText("")
        binding.arg7EditText.setText("")
        binding.arg6EditText.isEnabled = false
        binding.tvArg7.text = "No hay mas parametros"
        binding.arg7EditText.isEnabled = false

        binding.extraInfo.text = "GPIO A0:"
        binding.gpioEditText.setText(mq2Config.A0.toString())

        binding.gpioEditText2.visibility = View.GONE
        binding.gpio2TV.visibility = View.GONE

    }
    private fun verHcr(){
        binding.arg4EditText.setText(hcrs04cfg.uAguaSuperior.toString())
        binding.arg5EditText.setText(hcrs04cfg.uAguaInferior.toString())
        binding.tvArg4.text = "Umbral nivel agua superior:"
        binding.tvArg5.text = "Umbral nivel agua inferior:"
        binding.tvArg6.text = "Distancia sensor-tanque(cm):"
        binding.arg6EditText.setText(hcrs04cfg.tankSensorCm.toString())
        binding.arg7EditText.setText(hcrs04cfg.tankDepthCm.toString())
        binding.arg6EditText.isEnabled = true
        binding.tvArg7.text = "Profundidad tanque(cm):"
        binding.arg7EditText.isEnabled = true

        binding.gpioEditText2.visibility = View.VISIBLE
        binding.gpio2TV.visibility = View.VISIBLE

        binding.extraInfo.text = "GPIO Trig:"
        binding.gpio2TV.text = "GPIO Echo:"
        binding.gpioEditText.setText(hcrs04cfg.Trig.toString())
        binding.gpioEditText2.setText(hcrs04cfg.Echo.toString())


    }
    private fun verSoSen(){
        binding.arg4EditText.setText(soSenConfig.uHumedSuperior.toString())
        binding.arg5EditText.setText(soSenConfig.uHumedInferior.toString())
        binding.tvArg4.text = "Umbral nivel humedad suelo superior"
        binding.tvArg5.text = "Umbral nivel humedad suelo  gas inferior"
        binding.tvArg6.text = "No hay mas parametros"
        binding.arg6EditText.setText("")
        binding.arg7EditText.setText("")
        binding.arg6EditText.isEnabled = false
        binding.tvArg7.text = "No hay mas parametros"
        binding.arg7EditText.isEnabled = false

        binding.extraInfo.text = "GPIO PIN:"
        binding.gpioEditText.setText(soSenConfig.A0.toString())

        binding.gpioEditText2.visibility = View.GONE
        binding.gpio2TV.visibility = View.GONE
    }
    private fun inicializaSensores() {
        dht22cfg = DhtConfig(0, 0, 0, 0, 0, 0, 0)
        soSenConfig = SoSenConfig(0, 0, 0, 0, 0)
        mq2Config = Mq2Config(0, 0, 0, 0, 0)
        hcrs04cfg = Hcrs04Config(0, 0, 0, 0, 0, 0, 0, 0)
    }

    /********************Funcion para leer parametros de intent**************/
    private fun recibeVariablesIntent(){
        val intent = intent
        nombreEsp = intent.getStringExtra("nombre_esp").toString()
        Log.d("EditEsp", "Nombre esp: $nombreEsp")
        aliasEsp = intent.getStringExtra("alias_esp").toString()
        Log.d("EditEsp", "Alias esp: $aliasEsp")
    }


    /**Funcion de error y recepcion de mensajes en topico de consulta*/
    private fun onMqttErrorSuscribeConsulta(result: MqttResultado.Failure) {
        Toast.makeText(this@EditEspActivity, "Error al suscribir al topico mqtt, revisa conexion de los dispositivos", Toast.LENGTH_SHORT).show()
    }
    private fun onMqttMessageConsultaRec(result: MqttResultado.Success){
        result.payload?.run{

            val msg = String(this)
            val topicCurr = mqttData!!.getTopic()
            Log.d("MsgEdit", "Se ha recibido mensaje $msg")
            if(!topicCurr.contains("/USR/$userName/RESP"))
                return
            //Es un mensaje de confirmacion
            if(!msg.contains("UT1")){
                parseSetConf(msg)
            }
            else {
                if (tipoSensor == "DHT22") {
                    parseJSONdht22(msg)
                }
                if (tipoSensor == "MQ2") {
                    parseJSONmq2(msg)
                }
                if (tipoSensor == "HC-RS04") {
                    parseJSONhcrs(msg)
                }
                if (tipoSensor == "SO-SEN") {
                    parseJSONsoSen(msg)
                }
            }
        }
    }

    private fun parseSetConf(msg: String) {
        val jsonObject = JSONTokener(msg).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")
        val id = dt.getInt("ID")
        val res = dt.getInt("RES")
        if(res == 1){
            Toast.makeText(this@EditEspActivity, "Se ha configurado con exito", Toast.LENGTH_SHORT).show()
        }
        else if(res == -100){
            Toast.makeText(this@EditEspActivity, "El campo \"OPT\" no se ha definido o no es un numero", Toast.LENGTH_SHORT).show()
        }
        else if(res == -101){
            Toast.makeText(this@EditEspActivity, "El campo \"ARG1\" no se ha definido", Toast.LENGTH_SHORT).show()
        }
        else if(res == -102){
            Toast.makeText(this@EditEspActivity, "El campo \"ARG2\" no se ha definido", Toast.LENGTH_SHORT).show()
        }
        else if(res == -103){
            Toast.makeText(this@EditEspActivity, "El campo \"ARG3\" no se ha definido", Toast.LENGTH_SHORT).show()
        }
        else if(res == -104){
            Toast.makeText(this@EditEspActivity, "El campo \"ARG4\" no se ha definido", Toast.LENGTH_SHORT).show()
        }
        else if(res == -106){
            Toast.makeText(this@EditEspActivity, "No se ha encontrado el ID del Sensor", Toast.LENGTH_SHORT).show()
        }
        else if(res == -107){
            Toast.makeText(this@EditEspActivity, "No se ha encontrado el Sensor_data del Sensor", Toast.LENGTH_SHORT).show()
        }
        else if(res == -108){
            Toast.makeText(this@EditEspActivity, "No hay sensores instalados de ese tipo o se ha intentado modificar un numero de sensor incorrecto", Toast.LENGTH_SHORT).show()
        }
        else if(res == -109){
            Toast.makeText(this@EditEspActivity, "OPT 0: Se ha intentado modificar un número de valor que no existe en el TIPO de sensor", Toast.LENGTH_SHORT).show()
        }
        else if(res == -110){
            Toast.makeText(this@EditEspActivity, "OPT 2: No se ha encontrado una configuración de parámetros", Toast.LENGTH_SHORT).show()
        }else if(res == -111){
            Toast.makeText(this@EditEspActivity, "OPT2: El numero de sensor a modificar no se encuentra entre los instalados con el GET_PARAMETERS", Toast.LENGTH_SHORT).show()
        }
        else if(res == -404){
            Toast.makeText(this@EditEspActivity, "Tipo de operacion no disponible", Toast.LENGTH_SHORT).show()
        }
        else if(res == -1){
            Toast.makeText(this@EditEspActivity, "Error generico del driver del sensor", Toast.LENGTH_SHORT).show()
        }


    }

    /********************PARSEO DE JSONS RECIBIDOS PARA CADA SENSOR**************/
    private fun parseJSONdht22(mensaje: String){
        val jsonObject = JSONTokener(mensaje).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")
        val id = dt.getInt("ID")
        val res = dt.getInt("RES")
        Log.d("EditEsp", "RES = $res")
        if(res < 0){
            Toast.makeText(this@EditEspActivity, "Unidad consultada no esta instalada", Toast.LENGTH_SHORT).show()
        }
        else{
        //TODO parsear segun las clasess de los sensoresConfigs
            try {
                val ut1 = dt.getInt("UT1")
                val ut2 = dt.getInt("UT2")
                val lt1 = dt.getInt("LT1")
                val lt2 = dt.getInt("LT2")
                val data = dt.getInt("Data")
                dht22cfg = DhtConfig(id, res, ut1, lt1, lt2, ut2, data)
                Log.d("EditEsp", "Parseado $ut1, $ut2, $lt1, $lt2, $data")

                verDht()

                val en1 = dt.getString("EN1")
                binding.activarBtnEdit1.isChecked = en1 == "true"
                val en2 = dt.getString("EN2")
                binding.activarBtnEdit2.isChecked = en2 == "true"

            }catch(e: JSONException){
                Log.d("JSON", "Error parsing json")
            }
        }
    }
    private fun parseJSONmq2(mensaje: String){
        val jsonObject = JSONTokener(mensaje).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")
        val id = dt.getInt("ID")
        val res = dt.getInt("RES")
        Log.d("EditEsp", "RES = $res")
        if(res < 0){
            Toast.makeText(this@EditEspActivity, "Unidad consultada no esta instalada", Toast.LENGTH_SHORT).show()
        }
        else{
            //TODO parsear segun las clasess de los sensoresConfigs
            try {
                val ut1 = dt.getInt("UT1")
                val lt1 = dt.getInt("LT1")
                val a0 = dt.getInt("A0")
                mq2Config = Mq2Config(id, res, ut1, lt1, a0)

                verMq2()

                val en1 = dt.getString("EN1")
                binding.activarBtnEdit1.isChecked = en1 == "true"


            }catch(e: JSONException){
                Log.d("JSON", "Error parsing json")
            }
        }
    }
    private fun parseJSONhcrs(mensaje: String){
        val jsonObject = JSONTokener(mensaje).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")
        val id = dt.getInt("ID")
        val res = dt.getInt("RES")
        Log.d("EditEsp", "RES = $res")
        if(res < 0){
            Toast.makeText(this@EditEspActivity, "Unidad consultada no esta instalada", Toast.LENGTH_SHORT).show()
        }
        else{
            //TODO parsear segun las clasess de los sensoresConfigs
            try {
                val ut1 = dt.getInt("UT1")
                val lt1 = dt.getInt("LT1")
                val trig = dt.getInt("Trig")
                val echo = dt.getInt("Echo")
                val sensorTank = dt.getInt("Sensor - Tank (cm)")
                val tankDepth = dt.getInt("Tank depth (cm)")
                hcrs04cfg = Hcrs04Config(id, res, ut1, lt1, trig, echo, sensorTank, tankDepth)

                verHcr()
                val en1 = dt.getString("EN1")
                binding.activarBtnEdit1.isChecked = en1 == "true"

            }catch(e: JSONException){
                Log.d("JSON", "Error parsing json")
            }
        }
    }
    private fun parseJSONsoSen(mensaje: String){
        val jsonObject = JSONTokener(mensaje).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")
        val id = dt.getInt("ID")
        val res = dt.getInt("RES")
        Log.d("EditEsp", "RES = $res")
        if(res < 0){
            Toast.makeText(this@EditEspActivity, "Unidad consultada no esta instalada", Toast.LENGTH_SHORT).show()
        }
        else{
            //TODO parsear segun las clasess de los sensoresConfigs
            try {
                val ut1 = dt.getInt("UT1")
                val lt1 = dt.getInt("LT1")
                val a0 = dt.getInt("A0")
                soSenConfig = SoSenConfig(id, res, ut1, lt1, a0)

                verSoSen()

                val en1 = dt.getString("EN1")
                binding.activarBtnEdit1.isChecked = en1 == "true"

            }catch(e: JSONException){
                Log.d("JSON", "Error parsing json")
            }
        }
    }


    /********************FUNCION DE PUBLICAR JSON DE CONFIGURACION*********************/
    private fun guardarDatos(){
        getObjConfiguracion()
        Log.d("guardarDatos", "El objeto scan arg1: ${setConfObj.ARG1}, arg2: ${setConfObj.ARG2}, arg3: ${setConfObj.ARG3}, arg4: ${setConfObj.ARG4}")
        val stringPublicado = "{\"USER\":\"$userName\",\"ID\":$idMensaje,\"OPT\":${tipoOperacion},\"ARG1\":\"${setConfObj.ARG1}\",\"ARG2\":\"${setConfObj.ARG2}\","+
                "\"ARG3\":\"${setConfObj.ARG3}\",\"ARG4\":\"${setConfObj.ARG4}\"}"
        val topicP = "MESPF/SENS/$nombreEsp/$tipoSensor/$numSensor/CMD/SET"
        mqttData?.publishTopic(topicP, stringPublicado)




    }

    /*****************CREACION JSON PARA ENVIAR MENSAJE SET CONFIGURACION**************/
    private fun getObjConfiguracion(){
        setConfObj.ID = idMensaje
        ++idMensaje
        setConfObj.OPT = tipoOperacion
        var activa1: Int = 0
        var activa2: Int = 0
        if(binding.activarBtnEdit1.isChecked)
            activa1 = 1
        if(binding.activarBtnEdit2.isChecked)
            activa2 = 1
        Log.d("guardaEsp", "tipo operacion $tipoOperacion")
        if(tipoOperacion == 0){
            if(tipoSensor == "DHT22"){
                /**Mandamos el primer valor de alarma porque tiene 2 valores distintos, humedad y temperatura*/
                setConfObj.ARG1 = "0"
                setConfObj.ARG2 = activa1.toString()
                setConfObj.ARG3= binding.arg4EditText.text.toString()
                setConfObj.ARG4= binding.arg5EditText.text.toString()

                val stringPublicado = "{\"USER\":\"$userName\",\"ID\":$idMensaje,\"OPT\":${tipoOperacion},\"ARG1\":\"${setConfObj.ARG1}\",\"ARG2\":\"${setConfObj.ARG2}\","+
                        "\"ARG3\":\"${setConfObj.ARG3}\",\"ARG4\":\"${setConfObj.ARG4}\"}"
                val topicP = "MESPF/SENS/$nombreEsp/$tipoSensor/$numSensor/CMD/SET"
                Log.d("guardarDatos", "El objeto dht22 arg1: ${setConfObj.ARG1}, arg2: ${setConfObj.ARG2}, arg3: ${setConfObj.ARG3}, arg4: ${setConfObj.ARG4}")
                mqttData?.publishTopic(topicP, stringPublicado)

                /**Mandamos el segundo*/
                setConfObj.ARG1 = "1"
                setConfObj.ARG2 = activa2.toString()
                setConfObj.ARG3 = binding.arg6EditText.text.toString()
                setConfObj.ARG4 = binding.arg7EditText.text.toString()
            }
            else{
                setConfObj.ARG1 = "0"
                setConfObj.ARG2 = activa1.toString()
                setConfObj.ARG3= binding.arg4EditText.text.toString()
                setConfObj.ARG4= binding.arg5EditText.text.toString()
            }

        }
        else if(tipoOperacion == 1){
            if(tipoSensor == "HC-RS04"){
                setConfObj.ARG1 = binding.gpioEditText.text.toString()
                setConfObj.ARG2 = binding.gpioEditText2.text.toString()
                setConfObj.ARG3 = "0"
                setConfObj.ARG4 = "0"
            }
            else{
                setConfObj.ARG1 = binding.gpioEditText.text.toString()
                setConfObj.ARG2 = "0"
                setConfObj.ARG3 = "0"
                setConfObj.ARG4 = "0"
            }
        }
        else if(tipoOperacion == 2){
            if(tipoSensor == "HC-RS04"){
                setConfObj.ARG1 = binding.arg6EditText.text.toString()
                setConfObj.ARG2 = binding.arg7EditText.text.toString()
                setConfObj.ARG3 = "0"
                setConfObj.ARG4 = "0"
            }else{
                setConfObj.ARG1 = "0"
                setConfObj.ARG2 = "0"
                setConfObj.ARG3 = "0"
                setConfObj.ARG4 = "0"
            }
        }
        else{
            Toast.makeText(this@EditEspActivity, "Elige una operacion valida", Toast.LENGTH_SHORT).show()
        }
    }


    /*****************FIN**************/
    override fun onBackPressed() {
        super.onBackPressed()

        mqttData?.unsubscribe("MESPF/USR/$userName/RESP")
    }

}

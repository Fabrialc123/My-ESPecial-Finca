package com.example.tfg_boceto

import android.app.NotificationManager
import android.app.Service
import android.content.Context
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.Worker
import androidx.work.WorkerParameters
import io.reactivex.Scheduler
import org.eclipse.paho.client.mqttv3.*
import com.example.tfg_boceto.*
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import org.json.JSONObject
import org.json.JSONTokener

class MqttWorker(appContext: Context, workerParams: WorkerParameters) : Worker(appContext, workerParams) {

    companion object {
        const val NOTIFICATION_ID = 1
        const val CHANNEL_ID = "mqtt_channel"
    }
    private lateinit var mqttClient: MqttAsyncClient
    private lateinit var topicList: Array<String>
    private lateinit var server:String
    private lateinit var listaSensores: List<String>
    override fun doWork(): Result {
        val ipRec = inputData.getString("ip")
        val nameMqtt = inputData.getString("userMqtt")
        val passMqtt = inputData.getString("passMqtt")
        val nameNormal = inputData.getString("name")
        val listaJsonSensores = inputData.getString("listSensores")

        if(listaJsonSensores != null){
            listaSensores = Gson().fromJson(listaJsonSensores, object: TypeToken<List<String>>() {}.type)
        }



        val brokerUri = "tcp://mqtt.example.com:1883"
        val clientId = MqttClient.generateClientId()

        val mqttClient = MqttClient("tcp://"+ipRec, clientId, MemoryPersistence())
        mqttClient.setCallback(object : MqttCallbackExtended {
            override fun connectionLost(cause: Throwable?) {
            }

            override fun messageArrived(topic: String?, message: MqttMessage?) {
                mostrarMensajeNotificacion(message.toString())
            }

            override fun deliveryComplete(token: IMqttDeliveryToken?) {
            }

            override fun connectComplete(reconnect: Boolean, serverURI: String?) {
                // Handle successful connection to MQTT broker
                try {
                    // Subscribe to desired MQTT topic
                    mqttClient.subscribe("your/topic")



                    if(!listaSensores.isEmpty()){
                        for(sensor in listaSensores){
                            mqttClient.subscribe(sensor+"ALERT")
                        }
                    }


                } catch (e: MqttException) {
                    e.printStackTrace()
                }
            }
        })

        val options = MqttConnectOptions()
        options.userName = nameMqtt
        options.password = passMqtt?.toCharArray()


        try {
            mqttClient.connect(options)
        } catch (e: MqttException) {
            e.printStackTrace()
            return Result.failure()
        }

        // Mantén el Worker en ejecución hasta que se complete la conexión MQTT
        while (!mqttClient.isConnected) {
            if (isStopped) {
                mqttClient.disconnect()
                return Result.failure()
            }
            Thread.sleep(3000)
        }

        // Mantén el Worker en ejecución para recibir mensajes MQTT
        while (true) {
            if (isStopped) {
                mqttClient.disconnect()
                return Result.failure()
            }
            Thread.sleep(3000)
        }

        // Mantén el Worker en ejecución para recibir mensajes MQTT
        while (true) {
            if (isStopped) {
                mqttClient.disconnect()
                return Result.failure()
            }
            Thread.sleep(1000)

            // Programa el siguiente trabajo dentro del mismo Worker
            val nextWorkRequest = OneTimeWorkRequestBuilder<MqttWorker>().build()
            WorkManager.getInstance(applicationContext).enqueue(nextWorkRequest)
        }
    }

    private fun mostrarMensajeNotificacion(mensaje: String){
        // Crear la notificacion
        val notificationBuilder = NotificationCompat.Builder(applicationContext, "mqtt_channel")
            .setSmallIcon(R.drawable.notificacion_icon)
            .setContentTitle("ESP Finca")
            .setContentText(parseaAlarma(mensaje))
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .setAutoCancel(true)
            .build()

        val notificationManager = NotificationManagerCompat.from(applicationContext)
        notificationManager.notify(NOTIFICATION_ID, notificationBuilder)
    }
    private fun parseaAlarma(msg: String?): String{
        if(msg == null)
            return ""
        val jsonObject = JSONTokener(msg).nextValue() as JSONObject

        val dt = jsonObject.getJSONObject("DT")
        val id = dt.getString("DESC")

        return id
    }
}
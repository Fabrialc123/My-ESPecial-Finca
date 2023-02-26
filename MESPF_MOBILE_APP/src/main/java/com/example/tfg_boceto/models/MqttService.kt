package com.example.tfg_boceto.models

import android.app.*
import android.app.PendingIntent.getActivity
import android.content.Context
import android.content.Intent
import android.content.Intent.getIntent
import android.content.Intent.getIntentOld
import android.os.Build
import android.os.IBinder
import android.util.Log
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import com.example.tfg_boceto.*
import org.eclipse.paho.client.mqttv3.*

class MqttService : Service() {


    companion object {
        const val NOTIFICATION_ID = 1
        const val CHANNEL_ID = "mqtt_channel"
    }
    private lateinit var mqttClient: MqttAsyncClient
    private lateinit var topicList: Array<String>
    override fun onCreate() {
        super.onCreate()

        //val serverUri = intent.getStringExtra("serverUri")
        //val password = intent.getStringExtra("password")
        //val username = intent.getStringExtra("username")
        //val topics = intent.getStringArrayListExtra("topics")

        startMqtt(sensoresList)
    }



    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        // Crear la notificación de servicio de primer plano
        createNotification()
        val notification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("MqttService")
            .setContentText("Servicio de MQTT en ejecución")
            .setSmallIcon(R.drawable.notificacion_icon)
            .build()

        // Iniciar el servicio de primer plano
        startForeground(NOTIFICATION_ID, notification)

        // Llamar al método padre onStartCommand para manejar el inicio del servicio
        return super.onStartCommand(intent, flags, startId)
    }

    override fun onBind(p0: Intent?): IBinder? {
        return null
    }

    private fun startMqtt(topicNotification: MutableList<String>) {
        val mqttServerUri = "tcp://$MQTT_SERVER"
        val mqttClientId = MqttAsyncClient.generateClientId()
        mqttClient = MqttAsyncClient(mqttServerUri, mqttClientId, null)

        val options = MqttConnectOptions().apply {
            isAutomaticReconnect = true
            isCleanSession = false
            userName = userLogin
            password = userPassword.toCharArray()
        }

        mqttClient.connect(options, null, object : IMqttActionListener {
            override fun onSuccess(asyncActionToken: IMqttToken?) {
                Log.d("MQTTSERVICE", "Conectado con exito")
                subscribeToTopic(topicNotification)
            }

            override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                // Manejar la falla de conexión
                Log.d("MQTTSERVICE", "Conexion error")
            }
        })
    }

    private fun subscribeToTopic(topics: MutableList<String>) {


        for(tAct in topics){
            mqttClient.subscribe(tAct+"ALERT", 0, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    Log.d("MQTTSERVICE", "Suscrito con exito")
                }

                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    Log.d("MQTTSERVICE", "Error subscribe connect")
                }
            })
            mqttClient.setCallback(object : MqttCallbackExtended {
                override fun connectComplete(reconnect: Boolean, serverURI: String?) {
                    // Manejar la conexión completa
                }

                override fun connectionLost(cause: Throwable?) {
                    // Manejar la pérdida de conexión
                    Log.d("MQTTSERVICE", "Conexion perdida")
                }

                override fun messageArrived(topic: String?, message: MqttMessage?) {
                    // Manejar el mensaje recibido
                    Log.d("MQTTSERVICE", "Se ha recibido mensaje mqtt service")
                    showMessageNotification(message?.toString())
                }

                override fun deliveryComplete(token: IMqttDeliveryToken?) {
                    // Manejar la entrega completa
                }
            })
        }

    }

    private fun showMessageNotification(message: String?) {
        // Crear un PendingIntent para abrir la actividad principal cuando se hace clic en la notificación
        val pendingIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT
        )

        // Crear un canal de notificación para Android Oreo y versiones superiores
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channelId = "mqtt_channel"
            val channelName = "MQTT Channel"
            val channelDescription = "Canal para notificaciones de MQTT"
            val importance = NotificationManager.IMPORTANCE_DEFAULT
            val channel = NotificationChannel(channelId, channelName, importance).apply {
                description = channelDescription
            }
            val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(channel)
        }

        // Crear la notificacion
        val notificationBuilder = NotificationCompat.Builder(this, "mqtt_channel")
            .setSmallIcon(R.drawable.notificacion_icon)
            .setContentTitle("ESP Finca")
            .setContentText(message)
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)

        // Mostrar la notificación
        with(NotificationManagerCompat.from(this)) {
            notify(0, notificationBuilder.build())
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        mqttClient.disconnect()
    }


    private fun createNotification(): Notification {
        val channelId =
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                createNotificationChannel(CHANNEL_ID, "My Background Service")
            } else {
                ""
            }

        val notificationBuilder = NotificationCompat.Builder(this, channelId)
            .setContentTitle("MQTT Service")
            .setContentText("MQTT Service is running in the background")
            .setSmallIcon(R.drawable.notificacion_icon)

        return notificationBuilder.build()
    }

    @RequiresApi(Build.VERSION_CODES.O)
    private fun createNotificationChannel(channelId: String, channelName: String): String {
        val chan = NotificationChannel(
            channelId,
            channelName,
            NotificationManager.IMPORTANCE_NONE
        )
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(chan)
        return channelId
    }
}
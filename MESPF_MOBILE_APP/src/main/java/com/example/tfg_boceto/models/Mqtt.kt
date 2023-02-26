package com.example.tfg_boceto.models

import android.content.ContentValues.TAG
import android.content.Context
import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import org.eclipse.paho.client.mqttv3.*
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import java.util.*
import kotlin.reflect.KFunction1


class Mqtt(uri: String) {
    private var serverURI = uri
    private var currTopic: String = ""


    fun getTopic(): String {
        return currTopic
    }

    // Inicializa cliente mqtt con la uri indicada  un id de cliente aleatorio + persistencia de la libreria
    private val mqttClient: MqttAsyncClient =
        MqttAsyncClient("tcp://$serverURI", UUID.randomUUID().toString(), MemoryPersistence())

    val _mqttStatusLiveData: MutableLiveData<MqttResultado> = MutableLiveData()



    init {
        mqttClient.setCallback(object : MqttCallback {
            override fun connectionLost(cause: Throwable?) {
                _mqttStatusLiveData.postValue(MqttResultado.Failure(cause))
            }

            override fun messageArrived(topic: String, message: MqttMessage?) {
                //Cuando se reciben los mensajes
                // Tomamos el nombre del topico para saber que tipo de dispositivo nos esta enviando datos
                currTopic = topic

                _mqttStatusLiveData.postValue(MqttResultado.Success(message?.payload))
            }

            override fun deliveryComplete(token: IMqttDeliveryToken?) {
                _mqttStatusLiveData.postValue(MqttResultado.Success(token?.message?.payload))
            }

        })

        _mqttStatusLiveData.postValue(MqttResultado.Waiting)
    }

    fun connect(username:String, pass: String, onConnected: () -> Unit,  onError: MqttResultado.Failure.() -> Unit = {}) {
        val mqttConnectionOptions = MqttConnectOptions()
        mqttConnectionOptions.isAutomaticReconnect = true
        mqttConnectionOptions.isCleanSession = false
        //TODO descomentar
        mqttConnectionOptions.userName = username
        mqttConnectionOptions.password = pass.toCharArray()

        try {
            mqttClient.connect(mqttConnectionOptions, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    val disconnectedBufferOptions = DisconnectedBufferOptions()
                    disconnectedBufferOptions.isBufferEnabled = true
                    disconnectedBufferOptions.bufferSize = 100
                    disconnectedBufferOptions.isPersistBuffer = false
                    disconnectedBufferOptions.isDeleteOldestMessages = false

                    mqttClient.setBufferOpts(disconnectedBufferOptions)
                    onConnected()
                    _mqttStatusLiveData.postValue(MqttResultado.Success("Connected".toByteArray()))
                }

                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    MqttResultado.Failure(exception).onError()
                }

            })
        } catch (e: MqttException) {
            _mqttStatusLiveData.postValue(MqttResultado.Failure(e))
        }
    }

    fun disconnect(){
        mqttClient.disconnect()
    }
    fun suscribeTopic(topic: String): LiveData<MqttResultado>{
        _mqttStatusLiveData.postValue(MqttResultado.Waiting)

        mqttClient.subscribe(topic, 0, null, object: IMqttActionListener{
            override fun onSuccess(asyncActionToken: IMqttToken?) {
                _mqttStatusLiveData.postValue(MqttResultado.Success("Suscribed".toByteArray()))

                Log.d(TAG, "Subscribed to $topic")
            }

            override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {

                _mqttStatusLiveData.postValue(MqttResultado.Failure(exception))
            }

        })

        return _mqttStatusLiveData
    }
    
    fun publishTopic(topic: String, payload: String){
        _mqttStatusLiveData.postValue(MqttResultado.Waiting)
        mqttClient.publish(topic, MqttMessage(payload.toByteArray(Charsets.UTF_8)))
    }

    fun unsubscribe(topic: String) {
        try {
            mqttClient.unsubscribe(topic, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    Log.d(TAG, "Unsubscribed to $topic")
                }

                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    Log.d(TAG, "Failed to unsubscribe $topic")
                }
            })
        } catch (e: MqttException) {
            e.printStackTrace()
        }
    }

    fun connected(): Boolean {
        return mqttClient.isConnected()
    }



}





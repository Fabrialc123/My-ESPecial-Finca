package com.example.tfg_boceto

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.View
import com.example.tfg_boceto.databinding.ActivityLoginmqttBinding
import com.example.tfg_boceto.models.Mqtt
import com.example.tfg_boceto.models.MqttResultado


private lateinit var binding: ActivityLoginmqttBinding
private var mqttObj: Mqtt? = null
class LoginMqttActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityLoginmqttBinding.inflate(layoutInflater)
        setContentView(binding.root)

        mqttObj = Mqtt(MQTT_SERVER)

    }



    // Validation function
    private fun validateCredentials(): Boolean {
        // Check if username and password are valid

        val nombre = binding.username
        val pass = binding.password
        if (nombre.text.isNotEmpty()&& pass.text.isNotEmpty()) {
            return true
        }

        mqttObj?.connect(nombre.text.toString(),pass.text.toString(), ::onSuccessMqtt,::errorMqtt)

        //TODO view gone barra de login
        //TODO intnet para ir al main activity una vez se ha hecho el login con exito

        // Show error message if credentials are invalid
        //Toast.makeText(this, "Invalid username or password", Toast.LENGTH_SHORT).show()
        return false
    }

    private fun errorMqtt(result: MqttResultado.Failure){
        //TODO tratar error login
        binding.errorViewLogin.visibility = View.VISIBLE
        Thread.sleep(2000)
        finishLogin(false)
    }

    private fun onSuccessMqtt(){
        //TODO volver a la main activity y marcar que se ha podido conectar con exito
        binding.successViewLogin.visibility = View.VISIBLE
        Thread.sleep(2000)
        finishLogin(true)

    }

    private fun finishLogin(OK: Boolean){
        val intent = Intent(this, MainActivity::class.java)
        intent.putExtra("login", OK)
        startActivity(intent)
    }

    override fun onBackPressed() {
        super.onBackPressed()


    }
}
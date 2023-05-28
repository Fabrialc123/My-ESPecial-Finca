package com.example.tfg_boceto

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import androidx.lifecycle.lifecycleScope
import com.example.tfg_boceto.databinding.ActivityAddEspBinding
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch


private lateinit var nombreEsp: String
//val Context.dataStore by preferencesDataStore(name = "USER_PREFERENCES")



class AddEspActivity : AppCompatActivity(){
    lateinit var binding: ActivityAddEspBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)


        nombreEsp = savedInstanceState?.getString("nombre").toString()

        binding = ActivityAddEspBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.tvEspNombre.setText(nombreEsp)


        binding.btAddEsp.setOnClickListener{
            val data:Intent = Intent(this, MainActivity::class.java)
            val temperatura = binding.temperaturaSiNo.isChecked
            val humedad = binding.humedadSiNo.isChecked
            val agua = binding.aguaSiNo.isChecked
            lifecycleScope.launch(Dispatchers.IO){
                add_data_store(nombreEsp)
            }

            //setResult(Activity.RESULT_OK, data)
            //finish()
        }

    }

    private fun add_data_store(nombre: String){
        /*dataStore.edit { preferences ->
            preferences[stringPreferencesKey("nombre")] = nombre }
        */

    }

}
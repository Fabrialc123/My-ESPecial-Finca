package com.example.tfg_boceto

import android.app.Activity
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.View
import android.widget.Button
import com.example.tfg_boceto.databinding.ActivityAddEspBinding
import com.example.tfg_boceto.databinding.ActivityMainBinding

class AddEspActivity : AppCompatActivity(){
    lateinit var binding: ActivityAddEspBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityAddEspBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btAddEsp.setOnClickListener{
            val data:Intent = Intent()
            data.putExtra("nombre", binding.tvEspNombre.text.toString())
            data.putExtra("temperatura", binding.temperaturaSiNo.isChecked)
            data.putExtra("humedad", binding.humedadSiNo.isChecked)
            data.putExtra("agua", binding.aguaSiNo.isChecked)

            setResult(Activity.RESULT_OK, data)
            finish()
        }

    }

}
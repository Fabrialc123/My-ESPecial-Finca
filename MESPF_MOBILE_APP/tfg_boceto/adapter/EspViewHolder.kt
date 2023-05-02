package com.example.tfg_boceto.adapter

import android.graphics.Color
import android.util.Log
import android.view.View
import androidx.core.content.ContextCompat

import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import com.example.tfg_boceto.Esp32
import com.example.tfg_boceto.R
import com.example.tfg_boceto.databinding.ItemEsp32Binding

class EspViewHolder(view: View) : RecyclerView.ViewHolder(view) {

    val binding = ItemEsp32Binding.bind(view)
    private var esp32Model: Esp32? = null

    //return position
    fun render(esp32Model: Esp32,  onClickListener: (Int) -> Unit, onclickEdit: (Int) -> Unit) {
        this.esp32Model = esp32Model

        //SI alias es distinto se elige Alias
        //if(esp32Model.nombre_esp != esp32Model.alias_esp)
            binding.tvEspNombre.text = esp32Model.alias_esp
        //else
           // binding.tvEspNombre.text = esp32Model.nombre_esp

        //############TEMPERATURA ºC
        if(esp32Model.temperatura == -1000.0)
        {
            binding.tvEspTemperatura.visibility = View.GONE
            binding.tvEspTemperatura.text = ""
        }

        else{
            binding.tvEspTemperatura.visibility = View.VISIBLE
            binding.tvEspTemperatura.text = "Temperatura "+esp32Model.temperatura.toString()+" ºC"
        }
        //####HUMEDAD ############# %
        if(esp32Model.humedad == -1000.0){
            binding.tvEspHumedadAmbiente.visibility = View.GONE
            binding.tvEspHumedadAmbiente.text = ""
        }
        else{
            binding.tvEspHumedadAmbiente.visibility = View.VISIBLE
            binding.tvEspHumedadAmbiente.text = "Humedad Ambiente: "+esp32Model.humedad.toString()+" %"
        }

        //### CONCENTRACION GAS %
        if(esp32Model.concentracion_gas == -1000.0)
        {
            binding.tvEspHumo.visibility = View.GONE
            binding.tvEspHumo.text = ""
        }
        else{
            binding.tvEspHumo.visibility = View.VISIBLE
            binding.tvEspHumo.text = "Concentracion gas: "+esp32Model.concentracion_gas.toString()+" %"
        }

        //####NIVEL AGUA
        if(esp32Model.nivel_agua == -1000.0)
        {
            binding.tvEspNivelAgua.visibility = View.GONE
            binding.tvEspNivelAgua.text = ""
        }
        else {
            binding.tvEspNivelAgua.visibility = View.VISIBLE
            binding.tvEspNivelAgua.text = "Concentracion gas: " + esp32Model.nivel_agua.toString() + " %"
        }
        //####NIVEL HUMEDAD TIERRA
        if(esp32Model.humedad_tierra == -1000.0){
            binding.tvEspNivelAgua.visibility = View.GONE
            binding.tvEspNivelHumedadTierra.text = ""
        }
        else {
            binding.tvEspNivelHumedadTierra.visibility = View.VISIBLE
            binding.tvEspNivelHumedadTierra.text =
                "Concentracion gas: " + esp32Model.humedad_tierra.toString() + " %"
        }

        //Si no hay valores para ningun sensor, no estamos recibiendo informacion
        if(esp32Model.temperatura == -1000.0 && esp32Model.humedad == -1000.0 && esp32Model.concentracion_gas == -1000.0
            && esp32Model.humedad_tierra == -1000.0 && esp32Model.nivel_agua == -1000.0)
            binding.ivEstadoConexion.setImageResource(R.drawable.conexion_inexistente_imagen)
        else
            binding.ivEstadoConexion.setImageResource(R.drawable.conexion_buena_imagen)



        binding.botonRefrescarHolder.setOnClickListener {
                onclickEdit(adapterPosition)}

        //No se hace aqui porque usamos el listener del adapter
        /*
        itemView.setOnClickListener {
            onClickListener(adapterPosition)
        }
        */
        // If we want to load the image from the internet
        //Glide.with(estado.context).load(esp32.iv_estado_conexion).into(estado)

    }

    fun updateView() {
        esp32Model?.let { esp32 ->
            // Actualizar valores de la vista basados en los atributos del objeto Esp32
            if (esp32.temperatura == -1000.0) {
                binding.tvEspTemperatura.visibility = View.GONE
                binding.tvEspTemperatura.text = ""
            } else {
                binding.tvEspTemperatura.visibility = View.VISIBLE
                binding.tvEspTemperatura.text =
                    "Temperatura " + esp32.temperatura.toString() + " ºC"
            }

            //####HUMEDAD ############# %
            if (esp32.humedad == -1000.0) {
                binding.tvEspHumedadAmbiente.visibility = View.GONE
                binding.tvEspHumedadAmbiente.text = ""
            } else {
                binding.tvEspHumedadAmbiente.visibility = View.VISIBLE
                binding.tvEspHumedadAmbiente.text =
                    "Humedad Ambiente: " + esp32.humedad.toString() + " %"
            }

            //### CONCENTRACION GAS %
            if (esp32.concentracion_gas == -1000.0) {
                binding.tvEspHumo.visibility = View.GONE
                binding.tvEspHumo.text = ""
            } else {
                binding.tvEspHumo.visibility = View.VISIBLE
                binding.tvEspHumo.text =
                    "Concentracion gas: " + esp32.concentracion_gas.toString() + " %"
            }

            //####NIVEL AGUA
            if (esp32.nivel_agua == -1000.0) {
                binding.tvEspNivelAgua.visibility = View.GONE
                binding.tvEspNivelAgua.text = ""
            } else {
                binding.tvEspNivelAgua.visibility = View.VISIBLE
                binding.tvEspNivelAgua.text =
                    "Concentracion gas: " + esp32.nivel_agua.toString() + " %"
            }
            //####NIVEL HUMEDAD TIERRA
            if (esp32.humedad_tierra == -1000.0) {
                binding.tvEspNivelAgua.visibility = View.GONE
                binding.tvEspNivelHumedadTierra.text = ""
            } else {
                binding.tvEspNivelHumedadTierra.visibility = View.VISIBLE
                binding.tvEspNivelHumedadTierra.text =
                    "Concentracion gas: " + esp32.humedad_tierra.toString() + " %"
            }

        }
    }

    fun isAttributeChanged(attribute: String): Boolean{
        return when (attribute) {
            "temperatura" -> esp32Model?.temperatura != -1000.0
            "humedad" -> esp32Model?.humedad != -1000.0
            "concentracion_gas" -> esp32Model?.concentracion_gas != -1000.0
            "nivel_agua" -> esp32Model?.nivel_agua != -1000.0
            "humedad_tierra" -> esp32Model?.humedad_tierra != -1000.0
            else -> false
        }
    }
}
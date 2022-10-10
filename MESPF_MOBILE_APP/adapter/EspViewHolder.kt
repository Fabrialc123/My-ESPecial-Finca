package com.example.tfg_boceto.adapter

import android.view.View

import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import com.example.tfg_boceto.Esp32
import com.example.tfg_boceto.databinding.ItemEsp32Binding

class EspViewHolder(view: View) : RecyclerView.ViewHolder(view) {

    private val binding = ItemEsp32Binding.bind(view)

    //return position
    fun render(esp32Model: Esp32, onClickListener: (Esp32) -> Unit, onclickEdit: (Int) -> Unit) {


        binding.tvEspNombre.text = esp32Model.nombre_esp
        if(esp32Model.mostrar_temperatura)
            binding.tvEspTemperatura.text = esp32Model.temperatura.toString()+" ºC"
        else binding.tvEspTemperatura.visibility = View.GONE
        //if(esp32Model.mostrar_agua)
            //binding.tvEspHumedad.text = esp32Model.nivel_agua.toString()
        //else binding.tvEspHumedad.visibility = View.GONE
        if(esp32Model.mostrar_humedad)
            binding.tvEspHumedad.text = esp32Model.humedad.toString()+" %"
        else binding.tvEspHumedad.visibility = View.GONE

        binding.botonEditHolder.setOnClickListener { onclickEdit(adapterPosition) }

        // Accion que ocurre al seleccionar toda la celda del esp32 seleccionado
        itemView.setOnClickListener {
            onClickListener(esp32Model)
        }

        // If we want to load the image from the internet
        //Glide.with(estado.context).load(esp32.iv_estado_conexion).into(estado)

    }
}
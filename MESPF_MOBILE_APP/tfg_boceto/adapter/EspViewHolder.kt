package com.example.tfg_boceto.adapter

import android.graphics.Color
import android.view.View

import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import com.example.tfg_boceto.Esp32
import com.example.tfg_boceto.databinding.ItemEsp32Binding

class EspViewHolder(view: View) : RecyclerView.ViewHolder(view) {

    private val binding = ItemEsp32Binding.bind(view)
    var isSelected: Boolean = false
    set(value) {
        field = value
        itemView.setBackgroundColor(if(value) Color.parseColor("#CCCCCC") else Color.parseColor("#FFFFFF"))
    }
    //return position
    fun render(esp32Model: Esp32, onClickListener: (Esp32) -> Unit, onclickEdit: (Int) -> Unit) {


        binding.tvEspNombre.text = esp32Model.nombre_esp
        binding.tvEspTemperatura.text = esp32Model.temperatura.toString()+" ºC"
        binding.tvEspHumedad.text = esp32Model.humedad.toString()+" %"
        binding.botonEditHolder.setOnClickListener { onclickEdit(adapterPosition) }

        // Accion que ocurre al seleccionar toda la celda del esp32 seleccionado
        itemView.setOnClickListener {
            onClickListener(esp32Model)
        }

        // If we want to load the image from the internet
        //Glide.with(estado.context).load(esp32.iv_estado_conexion).into(estado)

    }
}
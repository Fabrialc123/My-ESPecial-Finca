package com.example.tfg_boceto.adapter

import android.view.View
import androidx.recyclerview.widget.RecyclerView
import com.example.tfg_boceto.databinding.ItemTopicScanBinding


class TopicViewHolder(view: View) : RecyclerView.ViewHolder(view) {

    private val binding = ItemTopicScanBinding.bind(view)

    //return position
    fun render(topic: String, onClickListener: (String) -> Unit) {

        binding.tvDispositivoScanId.text = topic
        itemView.setOnClickListener {
            //binding.bvDispositivoAdd.visibility = View.VISIBLE //hacemos visible el boton de añadir
            onClickListener(topic)
        }


    }

    //Para boton dentro de view holder
    /*
    fun button(item: String){
        contexto = super.itemView.context
        //Inicia database
        dataBase = AppDatabase.getDataBase(contexto)
        binding.bvDispositivoAdd.setOnClickListener{
            val intent = Intent(contexto, MainActivity::class.java)
            //intent.putExtra("nombre", item)
            //startActivity(contexto, intent, null)
        }
    }
    */


}

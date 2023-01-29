package com.example.tfg_boceto.adapter



import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.example.tfg_boceto.Esp32
import com.example.tfg_boceto.R
                                                    // funcion lambda que devuelve un objeto seleccionado del recyclerview
class EspAdapter(private val espList: List<Esp32>,
                 private val onClickListener:(Esp32)-> Unit,
                 private val onclickEdit:(Int) -> Unit) : RecyclerView.Adapter<EspViewHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): EspViewHolder {
        val layoutInflater = LayoutInflater.from(parent.context)
        return EspViewHolder(layoutInflater.inflate(R.layout.item_esp32, parent, false))
    }


    // amount of items to be seen on screen
    override fun getItemCount(): Int = espList.size


    override fun onBindViewHolder(holder: EspViewHolder, position: Int) {
        val item = espList[position]        // onclickDelete return position to delete
        holder.render(item, onClickListener, onclickEdit)
    }

}
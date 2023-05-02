package com.example.tfg_boceto.adapter



import android.graphics.Color
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import com.example.tfg_boceto.Esp32
import com.example.tfg_boceto.R
                                                    // funcion lambda que devuelve un objeto seleccionado del recyclerview
class EspAdapter(private val espList: List<Esp32>,
                 private val onClickListener:(Int)-> Unit,
                 private val onClickRefresh:(Int) -> Unit) : RecyclerView.Adapter<EspViewHolder>() {

    var lastSelectedPosition = RecyclerView.NO_POSITION


    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): EspViewHolder {
        val layoutInflater = LayoutInflater.from(parent.context)
        return EspViewHolder(layoutInflater.inflate(R.layout.item_esp32, parent, false))
    }


    // amount of items to be seen on screen
    override fun getItemCount(): Int = espList.size



    override fun onBindViewHolder(holder: EspViewHolder, position: Int) {
        val item = espList[position]

        holder.render(item, onClickListener, onClickRefresh)
        //holder.updateView()

        //val isChecked =(position == lastSelectedPosition)
        //holder.binding.boxSelection.isChecked = isChecked

        Log.d("Adapter", "posicion: $position, lastPos: $lastSelectedPosition, isSelected ")

        if(position == lastSelectedPosition){
            holder.binding.root.setBackgroundColor(Color.parseColor("#CCCCCC"))
        }
        else{
            holder.binding.root.setBackgroundColor(Color.parseColor("#FFFFFF"))
        }
        holder.binding.root.setOnClickListener {
            Log.d("Adapter", "Se ha clickado el elemento")
            val antigSelect = lastSelectedPosition
            lastSelectedPosition = position

            notifyItemChanged(antigSelect)

            notifyItemChanged(lastSelectedPosition)
            onClickListener(position)
        }
            //notifyDataSetChanged()

        //}
        //ULTIMA PARTE AÑADIDA PARA ACTUALIZAR LISTA AL CAMBIAR LOS ATRIBUTOS
        //holder.updateView()

    }


}
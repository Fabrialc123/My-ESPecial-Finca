package com.example.tfg_boceto.adapter

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Color
import android.view.LayoutInflater
import android.view.ViewGroup
import android.widget.LinearLayout
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import com.example.tfg_boceto.R

class TopicAdapter(
    private val topicList: List<String>,
    private val onClickListener: (String) -> Unit
) : RecyclerView.Adapter<TopicViewHolder>() {
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): TopicViewHolder {
        val layoutInflater = LayoutInflater.from(parent.context)
        return TopicViewHolder(layoutInflater.inflate(R.layout.item_topic_scan, parent, false))
    }
    var lastSelectedPosition = RecyclerView.NO_POSITION

    override fun onBindViewHolder(holder: TopicViewHolder, position: Int) {
        val item = topicList[position]



        holder.isSelected = (position == lastSelectedPosition)
        holder.itemView.setOnClickListener {
            lastSelectedPosition = position
            notifyDataSetChanged()
        }

        //var rowLinearlayout = holder.itemView.findViewById<LinearLayout>(R.id.scanRecyclerList)
        //rowLinearlayout.setBackgroundColor(Color.parseColor("#FFFFFF")) // todos los colores a blanco
        //holder.itemView.setBackgroundColor(Color.parseColor("#858B8E")) // el item seleccionado en gris

        holder.render(item, onClickListener)

        //holder.button(item)
    }

    override fun getItemCount(): Int {
        return topicList.size
    }

}
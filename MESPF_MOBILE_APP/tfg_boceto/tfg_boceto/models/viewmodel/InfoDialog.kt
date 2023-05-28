package com.example.tfg_boceto.models.viewmodel

import android.app.Dialog
import android.content.Context
import android.os.Bundle
import android.util.Log
import android.view.View
import com.example.tfg_boceto.databinding.DialogoStatusBinding
import com.example.tfg_boceto.databinding.InfoDialogBinding
import org.json.JSONException
import org.json.JSONObject
import org.json.JSONTokener

private lateinit var binding: InfoDialogBinding


class InfoDialog(context: Context): Dialog(context) {

    //TODO terminar esta clase
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //bind view to binding e inflarla
        binding = InfoDialogBinding.inflate(layoutInflater)
        setContentView(binding.root)

    }

    override fun onBackPressed() {
        super.onBackPressed()
    }
}
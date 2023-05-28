package com.example.tfg_boceto.models

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build




class ServiceRestarter: BroadcastReceiver() {
    override fun onReceive(p0: Context?, p1: Intent?) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            p0?.startForegroundService(Intent(p0, MqttService::class.java))
        } else {
            p0?.startService(Intent(p0, MqttService::class.java))
        }
    }
}
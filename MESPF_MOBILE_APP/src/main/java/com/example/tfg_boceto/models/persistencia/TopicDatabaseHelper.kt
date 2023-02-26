package com.example.tfg_boceto.models.persistencia

import android.app.DownloadManager.COLUMN_ID
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

class TopicDatabaseHelper(context: Context) : SQLiteOpenHelper(context, DATABASE_NAME, null, DATABASE_VERSION) {

    companion object {
        private const val DATABASE_NAME = "tfgdatabase.db"
        private const val DATABASE_VERSION = 2
        private const val TABLE_NAME = "TOPIC_TABLE"
        private const val COLUMN_NAME_TOPICO = "Nombre"
        private const val COLUMN_ALIAS = "Alias"
    }

    override fun onCreate(db: SQLiteDatabase?) {
        db?.execSQL("CREATE TABLE $TABLE_NAME ($COLUMN_NAME_TOPICO TEXT, $COLUMN_ALIAS TEXT);")
    }

    override fun onUpgrade(db: SQLiteDatabase?, oldVersion: Int, newVersion: Int) {
        // handle upgrading the database
    }
}
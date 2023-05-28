package com.example.tfg_boceto.models.persistencia

import android.app.DownloadManager.COLUMN_ID
import android.content.ContentValues
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import androidx.core.database.getStringOrNull

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

    fun updateAlias(name: String, alias: String): Int{
        val db = writableDatabase
        val values = ContentValues().apply {
            put(COLUMN_ALIAS, alias)
        }
        val filas = db.update(TABLE_NAME, values, "$COLUMN_NAME_TOPICO=?", arrayOf(name))
        db.close()

        return filas
    }

    fun getAliasDelNombre(nombre: String): String{
        val db = readableDatabase
        val query = "SELECT $COLUMN_ALIAS FROM $TABLE_NAME WHERE $COLUMN_NAME_TOPICO = ?"
        val cursor = db.rawQuery(query, arrayOf(nombre))
        var alias: String? = null

        if (cursor.moveToFirst()) {
            alias = cursor.getStringOrNull(cursor.getColumnIndex(COLUMN_ALIAS))
        }

        cursor.close()
        db.close()

        return alias.toString()
    }

}
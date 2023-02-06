package com.example.tfg_boceto

import android.annotation.SuppressLint
import android.content.Intent
import android.os.Bundle

import androidx.appcompat.app.AppCompatActivity
import android.view.Menu
import android.view.MenuItem
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts.StartActivityForResult
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.tfg_boceto.adapter.EspAdapter
import com.example.tfg_boceto.databinding.ActivityMainBinding
import com.example.tfg_boceto.models.Mqtt
import com.example.tfg_boceto.models.MqttResultado
import com.example.tfg_boceto.models.persistencia.TopicDatabaseHelper
import org.eclipse.paho.client.mqttv3.MqttClient


// Se pone /# para mostrar todos los subtopicos de ese topico

const val MQTT_SERVER = "188.127.160.18:1883"
//const val MQTT_SERVER = "broker.mqttdashboard.com"
//const val MQTT_SCAN = "MESPF/SENS/SCAN" //TOpico donde publicas tu nombre como user
const val MQTT_SENSORES = "MESPF/SENS/"
const val MQTT_EXTRA = "MESPF/SENS/+/+/+/INFO" //AÑADIR ESTO AL FINAL DEL TOPICO
var actualTopic: String = "" // inicia sin topico

class MainActivity : AppCompatActivity() {


    private lateinit var binding: ActivityMainBinding
    private var esp32MutableList: MutableList<Esp32> = mutableListOf<Esp32>() //la lista que vamos a modificar y que se cargara en el recycler
    private lateinit var adapter: EspAdapter
    private var mqttData: Mqtt? = null
    private var topicToConnect: String = ""
    private var userLogin: String = "MESPF_USER"
    private var userPassword: String = "MESPF_USER"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)
        initRecyclerView()

        mqttData = Mqtt(MQTT_SERVER)

        //Inicializar database
        readFromTopicDB()

    }

    private fun nothing(){}
    @SuppressLint("Range")
    private fun readFromTopicDB(){

        val dbAct = TopicDatabaseHelper(this)
        val db = dbAct.readableDatabase
        val tabla = "TOPIC_TABLE"

        val cursor = db.query(tabla, arrayOf("Nombre"), null,null,null,null,null)

        if(cursor.moveToFirst()){
            val nameColumnIndex = cursor.getColumnIndexOrThrow("Nombre")

            do{
                val name = MQTT_SENSORES + cursor.getString(nameColumnIndex)
                name.plus("/#")
                topicToConnect = name
                Toast.makeText(this@MainActivity, "El topico es" + topicToConnect, Toast.LENGTH_SHORT).show()
                if(topicToConnect != "")
                    mqttData?.connect(userLogin,userPassword,::subscribesTopic , ::onMqttError)


                val indexSlah =topicToConnect.lastIndexOf("/")
                val textAfterSlash = if(indexSlah != -1) topicToConnect.substring(indexSlah +1) else ""
                if(textAfterSlash != ""){
                    val newEsp = Esp32(textAfterSlash,textAfterSlash, 0.0, 0.0, 0.0, 0.0, 0.0);
                    if(!esp32MutableList.contains(newEsp))
                        esp32MutableList.add(newEsp)
                    adapter.notifyItemInserted(adapter.itemCount - 1)
                }
            }while(cursor.moveToNext())
        }
        cursor.close()
        db.close()
    }

    /***********************************************************************/
    //funciones mqtt
    private fun onMqttError(result: MqttResultado.Failure) {
        //TODO Tratar mensaje error en mqtt
        Toast.makeText(this, "Error mqtt connect", Toast.LENGTH_LONG).show()
    }


    private fun subscribesTopic() = runOnUiThread {
        mqttData?.suscribeTopic(topicToConnect)?.observe(this) { result ->
            when (result) {
                is MqttResultado.Failure -> onMqttError(result)
                is MqttResultado.Success -> onMqttMessageReceived(result)
                //TODO mostrar barra de carga de suscripcion a topic
                else -> {}
            }
        }
    }

    private fun onMqttMessageReceived(result: MqttResultado.Success) {
        result.payload?.run {
            //primero se recibe el nombre del topico y luego el contenido
            //por tanto hay que ver si tenemos nombre o contenido
            val msg = String(this)
            val topicCurr = mqttData!!.getTopic()

            if(topicCurr.contains("DHT22")  ||topicCurr.contains("MQ2")
                || topicCurr.contains("HC-RS04") ||topicCurr.contains("SO-SEN") ){
                //asignamos topico actual
                actualTopic = msg

            }


            //TODO parsear cada elemento para el formato correspondiente usando las clases del paquete sensores
            else{
                if(actualTopic.contains("DHT22")){

                }
                else if(actualTopic.contains("MQ2")){

                }
                else if(msg.contains("HC-RS04")){

                }else if(msg.contains("SO-SEN")){

                }
                else{
                    //No hacemos nada no es topico conocido
                }
            }

            //val newEsp = Esp32(actualTopic, 22.0, 12.0, 1.0, true, true, false);
            //esp32MutableList.add(newEsp)
            //adapter.notifyItemInserted(adapter.itemCount - 1) // notificamos al adapter que se añadio un item en la posicion final

        }

    }

    //Recycler View
    private fun initRecyclerView() {
        //Grid layout para varios items por fila
        // le pasamos nuestra lista mutable al adapter
        adapter = EspAdapter(espList = esp32MutableList, onClickListener = { esp32 ->
            onItemSelected(esp32)
        },
            onclickEdit = { position -> onEditedItem(position) })
        val manager = LinearLayoutManager(this)
        val decoration = DividerItemDecoration(this, manager.orientation)

        binding.RecyclerESP.layoutManager = manager
        binding.RecyclerESP.adapter = adapter
        binding.RecyclerESP.addItemDecoration(decoration)
    }

    private fun onItemSelected(esp32: Esp32) {
        Toast.makeText(this, esp32.nombre_esp, Toast.LENGTH_SHORT).show()
    }

    private fun onEditedItem(position: Int) {
        //TODO pantalla para edicion de los dispositivos
        Toast.makeText(this, "Elegiste la posicion" + position, Toast.LENGTH_SHORT).show()
    }

    //FUNCIONES DE LOS BOTONES DEL MENU SUPERIOR
    private fun creaEsp() {
        val intent = Intent(this, AddEspActivity::class.java)
        resultLauncher.launch(intent)

    }

    //TODO Pasar
    private fun scanEsp(){
        val intent = Intent(this, ScanActivity::class.java)
        intent.putExtra("username", userLogin)
        intent.putExtra("password", userPassword)
        resultLauncher.launch(intent)
    }

    private fun editaEsp() {}

    //resultado de la actividad AddEspActivity
    private var resultLauncher = registerForActivityResult(StartActivityForResult()) { result ->
        /*
        if (result.resultCode == Activity.RESULT_OK) {
            val name = result.data?.getStringExtra("nombre").toString()
            var tempSi = result.data!!.getBooleanExtra("temperatura", false)
            val humedadSi = result.data!!.getBooleanExtra("humedad", false)
            val aguaSi = result.data!!.getBooleanExtra("agua", false)

            //TODO recibir data actual del dispositivo esp
            val newEsp = Esp32(name, 22, 12, 1, tempSi, humedadSi, aguaSi);
            esp32MutableList.add(newEsp)
            adapter.notifyItemInserted(adapter.itemCount - 1) // notificamos al adapter que se añadio un item en la posicion final

        }
        */
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        // Inflate the menu; this adds items to the action bar if it is present.
        binding.toolbar.inflateMenu(R.menu.menu_main)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {

        when (item.itemId) {
            R.id.nav_add -> scanEsp()//creaEsp()
            R.id.nav_edit -> editaEsp()
        }
        return super.onOptionsItemSelected(item)

    }


}

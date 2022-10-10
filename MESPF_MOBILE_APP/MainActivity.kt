package com.example.tfg_boceto

import android.app.Activity
import android.content.Intent
import android.os.Bundle

import androidx.appcompat.app.AppCompatActivity
import android.view.Menu
import android.view.MenuItem
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts.StartActivityForResult
import androidx.annotation.MainThread
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import com.example.tfg_boceto.adapter.EspAdapter
import com.example.tfg_boceto.databinding.ActivityMainBinding
import com.example.tfg_boceto.models.Mqtt
import com.example.tfg_boceto.models.MqttResultado


// Se pone /# para mostrar todos los subtopicos de ese topico
const val MQTT_SERVER = "broker.hivemq.com"
const val MQTT_TOPIC = "TESTFABRI/#"

class MainActivity : AppCompatActivity() {


    private lateinit var binding: ActivityMainBinding
    private var esp32MutableList: MutableList<Esp32> =
        Esp32Provider.espList.toMutableList() //la lista que vamos a modificar y que se cargara en el recycler
    private lateinit var adapter: EspAdapter
    private var mqttData: Mqtt? = null


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)
        initRecyclerView()

        //TODO Establecer conexion con broker mqtt

        mqttData = Mqtt(MQTT_SERVER)
        mqttData?.connect(::subscribesTopic, ::onMqttError)

    }


    //funciones mqtt
    private fun onMqttError(result: MqttResultado.Failure) {
        //TODO Tratar mensaje error en mqtt
        Toast.makeText(this, "Error mqtt connect", Toast.LENGTH_LONG).show()
    }

    private fun subscribesTopic() = runOnUiThread {
        mqttData?.suscribeTopic(MQTT_TOPIC)?.observe(this) { result ->
            when (result) {
                is MqttResultado.Failure -> onMqttError(result)
                is MqttResultado.Success -> onMqttMessageReceived(result)
                //TODO mostrar barra de carga de suscripcion a topic
            }
        }
    }

    private fun onMqttMessageReceived(result: MqttResultado.Success) {
        result.payload?.run {
            //Toast.makeText(this@MainActivity, String(this), Toast.LENGTH_LONG).show()
            val newEsp = Esp32(String(this), 22, 12, 1, true, true, false);
            esp32MutableList.add(newEsp)
            adapter.notifyItemInserted(adapter.itemCount - 1) // notificamos al adapter que se añadio un item en la posicion final

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

    private fun editaEsp() {}

    //resultado de la actividad AddEspActivity
    private var resultLauncher = registerForActivityResult(StartActivityForResult()) { result ->
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
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        // Inflate the menu; this adds items to the action bar if it is present.
        binding.toolbar.inflateMenu(R.menu.menu_main)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {

        when (item.itemId) {
            R.id.nav_add -> creaEsp()
            R.id.nav_edit -> editaEsp()
        }
        return super.onOptionsItemSelected(item)

    }


}

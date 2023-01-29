package com.example.tfg_boceto

class Esp32Provider {
    companion object {
        val espList = listOf(Esp32("Gallinero", 30.0, 10.0, 0.0, true, true, false)
        , Esp32("Salon", 22.0, 15.0, -1.0,false, false, false),
            Esp32("Habitacion 1", 26.0, 15.0, 0.0,false, true, false),
            Esp32("Dormitorio", 24.0, 25.0, 0.0,true, false, false),
            Esp32("Lavabo", 12.0, 16.0, 0.0,true, true, false),
            Esp32("Trastero", 23.0, 64.0, 0.0,true, true, false),
            Esp32("Plantacion", 25.0, 53.0, 0.0,true, true, false),
        )
    }
}
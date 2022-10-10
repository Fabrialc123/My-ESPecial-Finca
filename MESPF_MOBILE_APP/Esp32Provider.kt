package com.example.tfg_boceto

class Esp32Provider {
    companion object {
        val espList = listOf(Esp32("Gallinero", 30, 10, 0, true, true, false)
        , Esp32("Salon", 22, 15, -1,false, false, false),
            Esp32("Habitacion 1", 26, 15, 0,false, true, false),
            Esp32("Dormitorio", 24, 25, 0,true, false, false),
            Esp32("Lavabo", 12, 16, 0,true, true, false),
            Esp32("Trastero", 23, 64, 0,true, true, false),
            Esp32("Plantacion", 25, 53, 0,true, true, false),
        )
    }
}
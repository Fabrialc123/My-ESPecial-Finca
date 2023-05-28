package com.example.tfg_boceto.models

sealed class MqttResultado() {

    object Waiting : MqttResultado()

    data class Failure(val exception: Throwable?) : MqttResultado()

    data class Success(val payload: ByteArray?) : MqttResultado() {
        override fun equals(other: Any?): Boolean {
            if (this === other) return true
            if (javaClass != other?.javaClass) return false

            other as Success

            if (payload != null) {
                if (other.payload == null) return false
                if (!payload.contentEquals(other.payload)) return false
            } else if (other.payload != null) return false

            return true
        }

        override fun hashCode(): Int {
            return payload?.contentHashCode() ?: 0
        }
    }

}
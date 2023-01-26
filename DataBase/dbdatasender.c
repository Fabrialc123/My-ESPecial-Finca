#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mysql.h>
#define	DB_SERVER	"localhost"
#define	DB_USER		"root"
#define	DB_PASS		"MESPF"
#define	DB_DB		"mespfdb"

#define MQTT_QOS	0
#define MQTT_BROKER	"localhost"
#define MQTT_PORT	1883
#define MQTT_KEEPALIVE	60

MYSQL *conn;

bool db_connection_init(){
	bool recon = 1;
	conn = mysql_init(NULL);

	fprintf(stdout,"Reconnect option: %d \n",recon);
	mysql_options(conn,MYSQL_OPT_RECONNECT, &recon);
	
	fprintf(stdout,"Connecting to DataBase");
	while(!mysql_real_connect(conn,DB_SERVER,DB_USER,DB_PASS, DB_DB, 0, NULL, 0)){
		fprintf(stdout,".");
	}
	fprintf(stdout,"\nConnected to DataBase \n");

	if (mysql_query(conn,"INSERT INTO boot VALUES (SYSDATE())")){
		fprintf(stderr,"%s \n", mysql_error(conn));
	}

	return true;
}

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
	int rc;
	printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if(reason_code != 0){
		mosquitto_disconnect(mosq);
	}

	rc = mosquitto_subscribe(mosq, NULL, "MESPF/SENS/+/+/+/INFO", MQTT_QOS);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		mosquitto_disconnect(mosq);
	}
	rc = mosquitto_subscribe(mosq, NULL, "MESPF/SENS/+/+/+/ALERT", MQTT_QOS);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		mosquitto_disconnect(mosq);
	}
}


/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	int i;
	bool have_subscription = false;

	for(i=0; i<qos_count; i++){
		printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
		if(granted_qos[i] <= 2){
			have_subscription = true;
		}
	}
	if(have_subscription == false){
		fprintf(stderr, "Error: All subscriptions rejected.\n");
		mosquitto_disconnect(mosq);
	}
}


/* Callback called when the client receives a message. */
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	char *query;
	fprintf(stdout,"%s %d %s\n", msg->topic, msg->qos, (char *)msg->payload);
	query = malloc((strlen(msg->topic) + strlen((char*)msg->payload)) + 128);

	memset(query,0,strlen(msg->topic)+strlen((char*)msg->payload) + 127);

	strcpy(query,"INSERT INTO publish(message,topic) VALUES (\'");
	strcat(query,(char*)msg->payload);
	strcat(query,"\',\'");
	strcat(query,msg->topic);
	strcat(query,"\')");

	mysql_ping(conn); // Reconnects if idle too long
	
	if(mysql_query(conn,query)){
		fprintf(stderr, "%s \n", mysql_error(conn));
	}

	free(query);
}


int main(int argc, char *argv[])
{
	struct mosquitto *mosq;
	int rc;

	/* Required before calling other mosquitto functions */
	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, NULL);
	if(mosq == NULL){
		fprintf(stderr, "Error: Out of memory.\n");
		return -1;
	}

	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_subscribe_callback_set(mosq, on_subscribe);
	mosquitto_message_callback_set(mosq, on_message);

	fprintf(stdout,"Connecting to MQTT Broker");
	rc = mosquitto_connect(mosq, MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE);
	while(rc != MOSQ_ERR_SUCCESS){
		rc = mosquitto_connect(mosq, MQTT_BROKER,MQTT_PORT, MQTT_KEEPALIVE);
		fprintf(stdout,".");
	}
	fprintf(stdout,"\nConnected to MQTT Broker \n");
	if (!db_connection_init()){
	 	return -1;
	}

	/* Run the network loop in a blocking call. The only thing we do in this
	 * example is to print incoming messages, so a blocking call here is fine.
	 *
	 * This call will continue forever, carrying automatic reconnections if
	 * necessary, until the user calls mosquitto_disconnect().
	 */
	mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_lib_cleanup();
	mysql_close(conn);
	return 0;
}

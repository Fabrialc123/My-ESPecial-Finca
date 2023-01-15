CREATE OR REPLACE PROCEDURE pr_publish_inserted(IN msg VARCHAR(255), IN topic VARCHAR(128), IN m_cod_publish INT) 
BEGIN 
DECLARE str_aux VARCHAR(255);
DECLARE int_aux INT;
DECLARE topic_aux VARCHAR(128);
DECLARE data_in VARCHAR(255);
DECLARE id_esp32 VARCHAR(30);
DECLARE id_sensor VARCHAR(20);
DECLARE num_sensor INT;	DECLARE tm TIMESTAMP;
DECLARE m_arg_1 VARCHAR(20);	DECLARE m_arg_2 VARCHAR(20);	DECLARE m_arg_3 VARCHAR(20);	DECLARE m_arg_4 VARCHAR(20);
DECLARE m_id_alert int; 	DECLARE m_descr VARCHAR(128);

DECLARE EXIT HANDLER FOR SQLEXCEPTION 
BEGIN 
	UPDATE publish set error = 1 where cod_publish = m_cod_publish;
SELECT 'ERROR!';
END;

SET  str_aux = substring(topic, 12); -- MESPF/SENS/
SET  int_aux = locate('/',str_aux);
SET  id_esp32 = substring(str_aux, 1, int_aux -1);

SET str_aux = substring(str_aux,int_aux+1); -- ESP32_XYZ/
SET int_aux = locate('/',str_aux);
SET id_sensor = substring(str_aux,1,int_aux-1);

SET str_aux = substring(str_aux,int_aux+1); -- SENSOR/
SET int_aux = locate('/',str_aux);
SET num_sensor = cast(substring(str_aux,1,int_aux-1) AS INT);

SET str_aux = substring(str_aux,int_aux+1); -- NUM_SENSOR/
SELECT id_esp32, id_sensor, num_sensor ,str_aux;

SET data_in = JSON_EXTRACT(msg, '$.DT');
SET tm = timestamp(str_to_date(JSON_UNQUOTE(JSON_EXTRACT(msg, '$.TS')), '%H:%i:%s %d/%m/%Y'));
SELECT data_in, tm;

if (str_aux like 'INFO') then
	SET m_arg_1 = JSON_UNQUOTE(JSON_EXTRACT(data_in,'$.1'));
	SET m_arg_2 = JSON_UNQUOTE(JSON_EXTRACT(data_in,'$.2'));
	SET m_arg_3 = JSON_UNQUOTE(JSON_EXTRACT(data_in,'$.3'));
	SET m_arg_4 = JSON_UNQUOTE(JSON_EXTRACT(data_in,'$.4'));
	select m_arg_1,m_arg_2,m_arg_3,m_arg_4 ;
	INSERT INTO info(id_esp32,id_sensor,num_sensor,arg_1,arg_2,arg_3,arg_4,dt_info,cod_publish)
	VALUES (id_esp32,id_sensor, num_sensor, m_arg_1, m_arg_2, m_arg_3, m_arg_4, tm, m_cod_publish);
else if (str_aux like 'ALERT') then
	SET m_id_alert = JSON_UNQUOTE(JSON_EXTRACT(data_in,'$.ID'));
	SET m_descr = JSON_UNQUOTE(JSON_EXTRACT(data_in,'$.DESC'));
	SELECT m_id_alert, m_descr;
	INSERT INTO alert(id_esp32,id_sensor,num_sensor,id_alert,descr,dt_info,cod_publish)
	VALUES (id_esp32,id_sensor, num_sensor, m_id_alert,m_descr, tm, m_cod_publish);
else 
end if;
END; //
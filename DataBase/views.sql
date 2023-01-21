CREATE OR REPLACE VIEW v_uptime AS
SELECT 	esp32.id_esp32 as ESP32,
		esp32.cod_publish_begin AS P_BGN,
		pubBGN.dt_publish as T_BGN,
		esp32.cod_publish_end as P_END,
		pubEND.dt_publish as T_END,
		case when esp32.ended = FALSE then NULL
			else  timediff(pubEND.dt_publish,pubBGN.dt_publish)
		end as TOTAL_TIME,
		(SELECT COUNT(*) FROM info WHERE cod_publish >= esp32.cod_publish_begin AND  cod_publish <= esp32.cod_publish_end AND id_esp32 = esp32.id_esp32) as NUM_INFOS,
		(SELECT COUNT(*) FROM alert WHERE cod_publish >= esp32.cod_publish_begin AND  cod_publish <= esp32.cod_publish_end AND id_esp32 = esp32.id_esp32) as NUM_ALERTS
FROM esp32, publish pubBGN, publish pubEND
WHERE esp32.cod_publish_begin = pubBGN.cod_publish AND esp32.cod_publish_end = pubEND.cod_publish
ORDER BY esp32.id_esp32
;

CREATE OR REPLACE VIEW v_last_info AS 
SELECT 	info.cod_publish as CP, 
		info.id_esp32 as ESP32, 
		info.id_sensor as SENSOR, 
		info.num_sensor as NUM, 
		info.arg_1 as I1,   
		info.arg_2 as I2, 
		info.arg_3 as I3, 
		info.arg_4 as I4, 
		info.dt_info as TS, 
		publish.dt_publish as T_RECEIVED
FROM info , publish
where info.cod_publish IN (SELECT MAX(cod_publish) FROM info GROUP BY id_esp32,id_sensor,num_sensor)
AND info.cod_publish = publish.cod_publish
;
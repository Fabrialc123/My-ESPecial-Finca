CREATE OR REPLACE VIEW v_last_info AS 
SELECT * 
FROM info 
where cod_info IN 
(SELECT MAX(cod_info) FROM info GROUP BY id_esp32,id_sensor,num_sensor)
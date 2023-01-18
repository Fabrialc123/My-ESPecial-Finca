CREATE OR REPLACE PROCEDURE pr_update_esp32_uptime (IN m_cod_publish INT, IN m_id_esp32 VARCHAR(30), IN uptime VARCHAR(20))
BEGIN
	DECLARE tm_bg TIMESTAMP;
	DECLARE tm_end TIMESTAMP;
	DECLARE last_uptime VARCHAR(20);
	DECLARE last_status INT;
	
	SELECT dt_publish INTO tm_bg FROM publish WHERE cod_publish = m_cod_publish;
	SELECT info.cod_publish, info.arg_4, publish.dt_publish INTO last_status, last_uptime, tm_end FROM esp32, info , publish
	WHERE esp32.id_esp32 = m_id_esp32 AND esp32.ended = FALSE 
	AND esp32.cod_publish_end = info.cod_publish
	AND publish.cod_publish = esp32.cod_publish_end;

	IF (last_status IS NULL) THEN
		INSERT INTO esp32 VALUES (m_id_esp32, m_cod_publish,m_cod_publish, FALSE);
	ELSEIF (last_uptime <= uptime) THEN	
		UPDATE esp32 SET cod_publish_end = m_cod_publish WHERE id_esp32 = m_id_esp32 AND ended = FALSE;
	ELSE
		UPDATE esp32 SET ended = TRUE WHERE id_esp32 = m_id_esp32 AND ended = FALSE;
		INSERT INTO esp32 VALUES (m_id_esp32,m_cod_publish,m_cod_publish, FALSE);
	END IF;

END;
//

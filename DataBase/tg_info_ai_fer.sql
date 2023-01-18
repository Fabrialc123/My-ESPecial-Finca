CREATE OR REPLACE TRIGGER tg_info_ai_fer AFTER INSERT ON info FOR EACH ROW
BEGIN
	IF NEW.ID_SENSOR = 'STATUS' THEN
		CALL pr_update_esp32_uptime(NEW.cod_publish, NEW.id_esp32, NEW.arg_4);
	END IF;
END;
//

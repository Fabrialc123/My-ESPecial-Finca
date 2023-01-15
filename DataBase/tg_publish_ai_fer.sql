CREATE OR REPLACE TRIGGER tg_publish_ai_fer AFTER INSERT ON publish FOR EACH ROW
BEGIN
	CALL pr_publish_inserted(NEW.message, NEW.topic, NEW.cod_publish);
END;
//
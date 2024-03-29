CREATE TABLE publish (
	cod_publish INT AUTO_INCREMENT PRIMARY KEY,
	message VARCHAR(200),
	topic VARCHAR(128),
	dt_publish TIMESTAMP DEFAULT SYSDATE()
);

CREATE TABLE publish_processed (
	cod_publish INT PRIMARY KEY,
	processed BOOLEAN NOT NULL DEFAULT FALSE,
	error BOOLEAN NOT NULL DEFAULT FALSE,
	CONSTRAINT FK_publish_processed_cod_publish FOREIGN KEY (cod_publish) REFERENCES publish(cod_publish) ON DELETE CASCADE
);

CREATE TABLE esp32 (
	id_esp32 VARCHAR(30) NOT NULL,
	cod_publish_begin INT NOT NULL,
	cod_publish_end INT,
	ended BOOLEAN DEFAULT FALSE,
	CONSTRAINT FK_esp32_cod_publish_begin FOREIGN KEY (cod_publish_begin) REFERENCES publish(cod_publish) ON DELETE CASCADE,
	CONSTRAINT FK_esp32_cod_publish_end FOREIGN KEY (cod_publish_end) REFERENCES publish(cod_publish) ON DELETE CASCADE
);

CREATE TABLE info (
	cod_publish INT PRIMARY KEY,
	id_esp32 VARCHAR(30) NOT NULL,
	id_sensor VARCHAR(20) NOT NULL,
	num_sensor INT NOT NULL,
	arg_1 VARCHAR(20),
	arg_2 VARCHAR(20),
	arg_3 VARCHAR(20),
	arg_4 VARCHAR(20),
	dt_info TIMESTAMP,
	CONSTRAINT FK_info_cod_publish FOREIGN KEY (cod_publish) REFERENCES publish(cod_publish) ON DELETE CASCADE
);

CREATE TABLE alert (
	cod_publish INT PRIMARY KEY,
	id_esp32 VARCHAR(30) NOT NULL,
	id_sensor VARCHAR(20) NOT NULL,
	num_sensor INT NOT NULL,
	id_alert INT,
	descr VARCHAR(128),
	dt_info TIMESTAMP,
	CONSTRAINT FK_alert_cod_publish FOREIGN KEY (cod_publish) REFERENCES publish(cod_publish) ON DELETE CASCADE
);

CREATE TABLE boot (
	time_boot TIMESTAMP NOT NULL
);

//


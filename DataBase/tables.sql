CREATE TABLE publish (
  cod_publish INT AUTO_INCREMENT NOT NULL PRIMARY KEY,
  message VARCHAR(200),
  topic VARCHAR(128),
  dt_publish TIMESTAMP DEFAULT SYSDATE(),
  processed BOOLEAN NOT NULL DEFAULT 0,
  error BOOLEAN NOT NULL DEFAULT 0
);

CREATE TABLE esp32 (
  id VARCHAR(30) NOT NULL,
  dt_begin TIMESTAMP NOT NULL,
  cod_publish_begin INT NOT NULL,
  dt_end TIMESTAMP,
  cod_publish_end INT,
  CONSTRAINT FK_esp32_cod_publish_begin FOREIGN KEY (cod_publish_begin) REFERENCES publish(cod_publish),
  CONSTRAINT FK_esp32_cod_publish_end FOREIGN KEY (cod_publish_end) REFERENCES publish(cod_publish)
);

CREATE TABLE info (
  cod_info INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  id_esp32 VARCHAR(30) NOT NULL,
  id_sensor VARCHAR(20) NOT NULL,
  num_sensor INT NOT NULL,
  arg_1 VARCHAR(20),
  arg_2 VARCHAR(20),
  arg_3 VARCHAR(20),
  arg_4 VARCHAR(20),
  dt_info TIMESTAMP,
  cod_publish INT NOT NULL,
  CONSTRAINT FK_info_cod_publish FOREIGN KEY (cod_publish) REFERENCES publish(cod_publish)
);

CREATE TABLE alert(
  cod_alert INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  id_esp32 VARCHAR(30) NOT NULL,
  id_sensor VARCHAR(20) NOT NULL,
  num_sensor INT NOT NULL,
  id_alert INT,
  descr VARCHAR(128),
  dt_info TIMESTAMP,
  cod_publish INT NOT NULL,
  CONSTRAINT FK_alert_cod_publish FOREIGN KEY (cod_publish) REFERENCES publish(cod_publish)
);

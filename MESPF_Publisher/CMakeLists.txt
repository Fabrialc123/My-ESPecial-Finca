# Edit following two lines to set component requirements (see docs)
idf_component_register(SRCS main.c wifi_app.c http_server.c
							mqtt/mqtt_app.c
							mqtt/mqtt_commands.c
							recollecter.c
							status.c
							tests.c
							gpios/led_rgb.c
							gpios/lcd.c
							gpios/mq2.c
							gpios/dht22.c
							gpios/hc_rs04.c
							gpios/so_sen.c
							gpios/rtc.c
							test_sntp.c
							log.c
						INCLUDE_DIRS "."
						EMBED_FILES webpage/app.css 
						webpage/app.js  
						webpage/index.html 
						webpage/jquery-3.3.1.min.js )

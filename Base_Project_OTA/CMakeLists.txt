# Edit following two lines to set component requirements (see docs)
idf_component_register(SRCS main.c wifi_app.c http_server.c mqtt.c
							gpios/led_rgb.c
						INCLUDE_DIRS "."
						EMBED_FILES webpage/app.css 
						webpage/app.js  
						webpage/index.html 
						webpage/jquery-3.3.1.min.js )

idf_component_register(SRCS  "main.c" "wifi_station.c" "ota.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ${project_dir}/cert/google.pem)
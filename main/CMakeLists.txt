idf_component_register(SRCS  "main.c" "wifi_station.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ${project_dir}/cert/google.pem)
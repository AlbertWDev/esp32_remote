idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES certs/cacert.pem certs/prvtkey.pem)

# spiffs_create_partition_image(storage ./spiffs FLASH_IN_PROJECT)
spiffs_create_partition_image(storage ./spiffs)
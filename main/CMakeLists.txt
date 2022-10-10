set(COMPONENT_SRCS "ota.c" "sub_pub.c" "wifi-connect.c" 
                   "device_drivers.c" "dht11.c" "main.c"
                   "ota.c")

set(COMPONENT_ADD_INCLUDEDIRS "") 

register_component()

target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate.pem.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private.pem.key" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/endpoint.txt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/github_server.cert" TEXT)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")

set(MySQL_INCLUDE_DIR "C:\\Program Files\\MySQL\\MySQL Connector C++ 9.2\\include\\jdbc")
set(MySQL_LIB_DIR "C:\\Program Files\\MySQL\\MySQL Connector C++ 9.2\\lib64\\vs14")

set(pcap_INCLUDE_DIR "C:\\Program Files\\Npcap\\npcap-sdk-1.13\\Include")
set(pcap_LIB_DIR "C:\\Program Files\\Npcap\\npcap-sdk-1.13\\Lib\\x64")

set(OpenSSL_INCLUDE_DIR "C:\\Program Files\\OpenSSL-Win64\\include")
set(OpenSSL_LIB_DIR "C:\\Program Files\\MySQL\\MySQL Connector C++ 9.2\\lib64\\vs14")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

set(MySQL_INCLUDE_DIR "")
set(MySQL_LIB_DIR "")

set(pcap_INCLUDE_DIR "")
set(pcap_LIB_DIR "")

set(OpenSSL_INCLUDE_DIR "")
set(OpenSSL_LIB_DIR "")

endif()
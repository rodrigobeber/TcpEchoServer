NOTES
-----

* Change default port as needed in main.cpp. The default is 8080.
* To use single thread, set poolSize = 0 in main.cpp. Default is the number of CPU cores available.
* To disable decryption, comment "#define USE_XOR_CIPHER" in XORCipher.h. Default is enabled.
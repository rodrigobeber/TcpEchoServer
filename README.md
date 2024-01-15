THE SERVER
----------

* Monitors multiple clients with select().
* Initializes non-blocking TCP sockets.
* Utilizes a thread pool for parallel processing the requests.
* Uses message serialization / deserialization.
* Decyphers messages using a XOR Cipher.

CLASSES
-------

* Connection: Handles socket communication. Receive and send the desired ammout of data based on the vector size.
* ThreadPool: Provides a pool of threads for handling concurrent execution of tasks.
* XORCipher: Implements encryption and decryption using XOR-based cryptography (the server uses only decryption).
* EchoMessageHandler: Handles echo messages serialization / deserialization. Also include debug routines.
* Protocol: Defines the rules and structure of communication between the server and clients.
* ClientHandler: Manages individual client connections, processing their requests and responses by using the Protocol
* Server: The main server class that initializes, manages, and coordinates client connections and communication.

NOTES
-----

* The default listen port is 8080. You can change it in main.cpp
* The default thread pool size is the number of CPU cores available. You can change it in main.cpp
* The default is to decrypt echo request messages. To disable decryption, comment "#define USE_XOR_CIPHER" in XORCipher.h

COMPILE & TEST
--------------
* Build and run the server: in the main project dir exec: mkdir build ; cd build ; cmake .. ; make ; ./echo-server
* Build and run my client test: in the main project dir exec: cd test ; ./make-client ; ./client
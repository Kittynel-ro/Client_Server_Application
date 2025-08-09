Build process:

$ mkdir build
$ cd build
$ run cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
$ make

How to run:

To start the server simply execute the file.
$ ./server.

For the client you will execute the file and provide the username and password as parameters.
$ ./client knock knock

After the login you can send messages to the echo server.

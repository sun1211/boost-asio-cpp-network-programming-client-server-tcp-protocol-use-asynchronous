# Boost Asio C++ Network Programming Client Server
Communicates with client/server applications over TCP protocol uses the asynchronous I/O and control operations

[Implementing an asynchronous TCP server](server/README.md)

[Implementing an asynchronous TCP client](client/README.md)

# Setup environment
**1. Install CMake**
```
cd ~
wget https://github.com/Kitware/CMake/releases/download/v3.14.5/cmake-3.14.5.tar.gz
tar xf cmake-3.14.5.tar.gz
cd cmake-3.14.5
./bootstrap --parallel=10
make -j4
sudo make -j4 install
```
**2. Install Boost**
```
cd ~
wget https://boostorg.jfrog.io/artifactory/main/release/1.69.0/source/boost_1_69_0.tar.gz
tar xf boost_1_69_0.tar.gz
cd boost_1_69_0
./bootstrap.sh
./b2 ... cxxflags="-std=c++0x -stdlib=libc++" linkflags="-stdlib=libc++" ...
sudo ./b2 toolset=gcc -j4 install
```

# How to build
```
mkdir build
cd build
cmake ..
cmake --build .
```

# How to Run
## Run server
```
./bin/server

```
we can check that server is run or not
```
netstat -tulpn | grep LISTEN

tcp        0      0 0.0.0.0:445             0.0.0.0:*               LISTEN      -
tcp        0      0 0.0.0.0:3333            0.0.0.0:*               LISTEN      6866/./bin/server   <===============
tcp        0      0 0.0.0.0:139             0.0.0.0:*               LISTEN      -
tcp        0      0 127.0.0.53:53           0.0.0.0:*               LISTEN      -
tcp        0      0 0.0.0.0:22              0.0.0.0:*               LISTEN      -
tcp        0      0 127.0.0.1:631           0.0.0.0:*               LISTEN      -
tcp        0      0 127.0.0.1:5433          0.0.0.0:*               LISTEN      -
tcp6       0      0 :::445                  :::*                    LISTEN      -
tcp6       0      0 :::5000                 :::*                    LISTEN      -
tcp6       0      0 :::5001                 :::*                    LISTEN      -
tcp6       0      0 :::139                  :::*                    LISTEN      -
tcp6       0      0 :::22                   :::*                    LISTEN      -
tcp6       0      0 ::1:631                 :::*                    LISTEN      -
```

## Run client
```
./bin/client

Request #1 has completed. Response: Response from server
```

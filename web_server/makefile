CXX = g++
EXEC_HTTP = server_http
EXEC_HTTPS = server_https

SOURCE_HTTP = main_http.cpp
SOURCE_HTTPS = main_https.cpp

OBJECTS_HTTP = main_http.o
OBJECTS_HTTPS = main_https.o

# 开启编译器O3优化，pthread启用多线程支持
LDFLAGS_COMMON = -std=c++11 -O3 -pthread -lboost_system
LDFLAGS_HTTP = 
LDFLAGS_HTTPS = -lssl -lcrypto

LPATH_COMMON = -I/usr/include/boost
LPATH_HTTP =
LPATH_HTTPS = -I/usr/bin/openssl

LLIB_COMMON = -L/usr/lib
LLIB_HTTP = 

all:
	make http
	make https

http:
	$(CXX) $(SOURCE_HTTP) $(LDFLAGS_COMMON) $(LDFLAGS_HTTP) $(LPATH_COMMON) $(LPATH_HTTP) $(LLIB_COMMON) $(LLIB_HTTP) -o $(EXEC_HTTP)
https:
	$(CXX) $(SOURCE_HTTPS) $(LDFLAGS_COMMON) $(LDFLAGS_HTTPS) $(LPATH_COMMON) $(LPATH_HTTPS) $(LLIB_COMMON) $(LLIB_HTTPS) -o $(EXEC_HTTPS)

clean:
	rm -f $(EXEC_HTTP) $(EXEC_HTTPS) *.o

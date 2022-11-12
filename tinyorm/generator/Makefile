BUILD = ../../build
LDFLAGS += -L${BUILD}/tinyorm/mysql4cpp -L../mysql4cpp/spdlog/lib
CFLAGS += -I.. -I../mysql4cpp/spdlog/include -I/usr/include/mysql -I../json-3.11.2/include

modelGen: ModelGen.cpp
	g++ ModelGen.cpp -o modelGen ${CFLAGS} ${LDFLAGS} -lmysql4cpp -lmysqlclient -lspdlog -std=c++17

clean:
	rm -f modelGen
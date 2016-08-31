ifeq ($(os),macx)
TARGET=libQSavefile.dylib
CXX=g++
else
TARGET=libQSavefile.so
CXX=arm-none-linux-gnueabi-g++
endif

CXXFLAGS= -pipe -O2 -Wall -W -fPIC
ifeq ($(os),macx)
LFLAGS = -dynamiclib
else
LFLAGS = -shared
endif



INCLUDEPATH += -I../QSlidingWindow -I../QSlidingWindowConsume

ifeq ($(os),macx)
LIBS += -L/usr/local/lib -lQSlidingWindow -lQSlidingWindowConsume -lsqlite3
else
LIBS += -L/usr/local/lib -lQSlidingWindow -lQSlidingWindowConsume
LIBS += -L/opt/sqlite3/lib -lsqlite3
INCLUDEPATH += -I/opt/sqlite3/include
endif

$(TARGET):QSavefile.o
	$(CXX) $(LFLAGS) -o $(TARGET) QSavefile.o $(LIBS)

QSavefile.o:QSavefile.cpp QSavefile.h
	$(CXX) -c $(CXXFLAGS) $(INCLUDEPATH) -o QSavefile.o QSavefile.cpp


clean:
	rm -f *.o $(TARGET)

install:
	cp -f $(TARGET) /usr/local/lib

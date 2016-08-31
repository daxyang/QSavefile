TARGET=libQSavefile.dylib

CXXFLAGS= -pipe -O2 -Wall -W -fPIC
LFLAGS = -dynamiclib
LIBS += -L/usr/local/lib -lQSlidingWindow -lQSlidingWindowConsume -lsqlite3
INCLUDEPATH += -I../QSlidingWindow -I../QSlidingWindowConsume
CXX=g++

$(TARGET):QSavefile.o
	$(CXX) $(LFLAGS) -o $(TARGET) QSavefile.o $(LIBS)

QSavefile.o:QSavefile.cpp QSavefile.h
	$(CXX) -c $(CXXFLAGS) $(INCLUDEPATH) -o QSavefile.o QSavefile.cpp


clean:
	rm -f *.o $(TARGET)

install:
	cp -f $(TARGET) /usr/local/lib

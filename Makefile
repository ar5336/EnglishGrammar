CC = g++
PROJECT = reader
SRC = reader.cpp
LIBS = `pkg-config --cflags --libs opencv4`
$(PROJECT) : $(SRC)
	$(CC) $(SRC) -o $(PROJECT) $(LIBS) -I /usr/include/boost_1_84_0

CC = g++

# put -Wall back when you're ready
# -Wextra when you're very ready
CFLAGS =  -I /usr/include/opencv4 -I /usr/include/boost_1_84_0
LDFLAGS = `pkg-config --cflags --libs opencv4`

SOURCES = main.cpp frames.cpp grammar.cpp grammar_reader.cpp string_operators.cpp parser.cpp displayer.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = myprogram

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

.cpp.o:
	$(CC) $(LDFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

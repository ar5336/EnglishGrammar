CC = g++ 
#-g -rdynamic

# put -Wall back when you're ready
# -Wextra when you're very ready
CFLAGS =  -I /usr/include/opencv4 -I /usr/include/boost_1_84_0 
LDFLAGS = `pkg-config --cflags --libs opencv4` 

SOURCES = $(shell find . -name '*.cpp')
EXECUTABLE = parser
BIN = bin
OBJECTS = $(patsubst %.cpp,$(BIN)/%.o,$(SOURCES))

EXECUTABLE_FILES = $(EXECUTABLE:%=$(BIN)/%)

build: $(EXECUTABLE_FILES)

all: $(EXECUTABLE)

$(EXECUTABLE_FILES): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)
	@echo "Build successful"

# Rule for building object files
$(OBJECTS): $(BIN)/%.o: %.cpp
	@mkdir -p $(dir $@)  # Ensure the directory exists
	$(CC) $(LDFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

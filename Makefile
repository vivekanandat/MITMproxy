CXX = g++
CXXFLAGS = -std=c++20 -lssl -lcrypto

TARGET = sp
SRC = $(wildcard *.cpp)
packets:
	mkdir -p packets

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX)  $(SRC) $(CXXFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)
	
clean:
	rm -f $(TARGET)

.PHONY: all run clean

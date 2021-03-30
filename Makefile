CC = g++
LD = g++

CFLAGS = -g -Wall -std=c++17

LDFLAGS = -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswscale -lswresample -lpthread

SRC = $(wildcard *.cpp)
OBJ = $(patsubst %cpp, %o, $(SRC))

TARGET = HalfwayMedia

.PHONY: all clean
all: $(TARGET)

$(TARGET) : $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o:%.cpp
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJ) $(TARGET)

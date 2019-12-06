APP_NAME = test.app
CC = g++
VULKAN_SDK_PATH = $(VULKAN_SDK)

CFLAGS = -std=c++2a -I$(VULKAN_SDK_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

SOURCE = main.cpp
OBJECTS = $(SOURCE:.cpp=.o)

all: prepere $(APP_NAME)

$(APP_NAME): $(OBJECTS)
	$(CC) -o $(APP_NAME) build/$(OBJECTS) $(CFLAGS) $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o build/$@

.PHONY: prepere test clean

prepere:
	mkdir -p build

test: $(APP_NAME)
	./$(APP_NAME)

clean:
	rm -r build
	rm -f $(APP_NAME)
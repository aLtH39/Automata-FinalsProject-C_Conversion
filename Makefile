CC     = gcc
CFLAGS = -Wall -Wextra `pkg-config --cflags gtk+-3.0`
LIBS   = `pkg-config --libs gtk+-3.0`
TARGET = validator

all: $(TARGET)

$(TARGET): main.c dfa.c tokenizer.c dfa.h tokenizer.h
	$(CC) main.c dfa.c tokenizer.c -o $(TARGET) $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET).exe $(TARGET)
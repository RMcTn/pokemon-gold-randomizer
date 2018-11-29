CC = g++
CFLAGS = -std=c++11 -g -Wall -Wextra

TARGET = pokemon-random

SRCDIR = src
OBJDIR = obj
SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

pokemon-random: $(OBJ)
		$(CC) $(CFLAGS)	-o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
		$(CC) -o $@ -c $< $(CFLAGS)

clean:
		rm -f $(OBJ) $(TARGET)

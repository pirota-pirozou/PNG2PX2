CC = gcc
CFLAGS = -Wall -Os
LDFLAGS = -lz -lpng
LIBS =
INCLUDE = -I./
TARGET = $(notdir $(CURDIR))
SRCDIR = ./
OBJDIR = ./obj

ifeq ($(strip $(OBJDIR)),)
OBJDIR = .
endif

.PHONY: clean

all: ./PX22PNG.exe ./PNG2PX2.exe

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS1 = $(addprefix $(OBJDIR)/, PNG2PX2.o pngctrl.o)
DEPENDS = $(OBJECTS1:.o=.d)
OBJECTS2 = $(addprefix $(OBJDIR)/, PX22PNG.o pngctrl.o)
DEPENDS2 = $(OBJECTS2:.o=.d)

PNG2PX2.exe: $(OBJECTS1) $(LIBS)
	$(CC) -o $@ $^ $(LDFLAGS)

PX22PNG.exe: $(OBJECTS2) $(LIBS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

clean:
	rm -f $(OBJECTS1) $(OBJECT2) $(DEPENDS) $(DEPENDS2)

# DO NOT DELETE THIS LINE – make depend depends on it.

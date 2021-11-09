DEPS = args_parser.h

OBJDIR = obj
OBJFILES = server.o args_parser.o qr_decoder.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(OBJFILES))

.PHONY: all
all: QRServer

$(OBJDIR)/%.o: %.cpp $(DEPS)
	g++ -c -o $@ $<

QRServer: $(OBJ)
	g++ -o $@ $^

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o QRServer
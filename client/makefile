DEPS = 

OBJDIR = obj
OBJFILES = client.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(OBJFILES))

.PHONY: all
all: client

$(OBJDIR)/%.o: %.cpp $(DEPS)
	g++ -c -o $@ $<

client: $(OBJ)
	g++ -o $@ $^

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o client
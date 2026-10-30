# Phos build
#
#   make              build everything
#   make run          build and run the echo server
#   make DEFS=-DSTCP_SERVER_PORT=9000   override macros (run make clean first)
#   make clean        remove build/

CC       = gcc
CXX      = g++
CFLAGS   = -Wall -Wextra -g
CXXFLAGS = -Wall -Wextra -g
CPPFLAGS = -Itcp
LDFLAGS  =

BUILD = build

# libphostcp: TCP server, static lib
PHOSTCP_SRCS = tcp/phos_tcp.c tcp/client.c
PHOSTCP_OBJS = $(PHOSTCP_SRCS:%.c=$(BUILD)/%.o)
LIBPHOSTCP   = $(BUILD)/libphostcp.a

ECHO_SERVER = $(BUILD)/echo-server
EXAMPLES    = $(BUILD)/examples/echo $(BUILD)/examples/ping
HTTP        = $(BUILD)/phos-http

.PHONY: all lib server examples http run clean

# keep example .o files instead of deleting them as intermediates
.SECONDARY:

all: lib server examples http

lib: $(LIBPHOSTCP)
server: $(ECHO_SERVER)
examples: $(EXAMPLES)
http: $(HTTP)

$(LIBPHOSTCP): $(PHOSTCP_OBJS)
	ar rcs $@ $^

$(ECHO_SERVER): $(BUILD)/tcp/server.o $(LIBPHOSTCP)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD)/examples/%: $(BUILD)/tcp/examples/%.o
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^

$(HTTP): $(BUILD)/http/main.o $(LIBPHOSTCP)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(ECHO_SERVER)
	./$(ECHO_SERVER)

clean:
	rm -rf $(BUILD)

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

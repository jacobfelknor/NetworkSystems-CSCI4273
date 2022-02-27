CC=gcc
CFLAGS=-g

BIN     := bin
SRC     := src
INCLUDE := include

all: server 

server: $(BIN)/http_server
# client: $(BIN)/udp_client

clean:
	rm -f $(BIN)/*

$(BIN)/http_server: $(SRC)/http_server.c $(SRC)/utils.c $(SRC)/str_utils.c
	$(CC) $(CFLAGS) -I$(INCLUDE) $^ -o $@

# $(BIN)/udp_client: $(SRC)/udp_client.c $(SRC)/str_utils.c $(SRC)/file_transfer.c $(SRC)/utils.c
# 	$(CC) $(CFLAGS) -I$(INCLUDE) $^ -o $@
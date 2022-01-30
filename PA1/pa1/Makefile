CC=gcc
CFLAGS=-I.

BIN     := bin
SRC     := src
INCLUDE := include

all: server client

server: $(BIN)/udp_server
client: $(BIN)/udp_client

clean:
	rm -f $(BIN)/*

$(BIN)/udp_server: $(SRC)/udp_server.c $(SRC)/str_utils.c $(SRC)/file_transfer.c $(SRC)/utils.c
	$(CC) -I$(INCLUDE) $^ -o $@

$(BIN)/udp_client: $(SRC)/udp_client.c $(SRC)/str_utils.c $(SRC)/file_transfer.c $(SRC)/utils.c
	$(CC) -I$(INCLUDE) $^ -o $@
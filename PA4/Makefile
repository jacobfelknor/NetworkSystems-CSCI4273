CC=gcc
CFLAGS=-g -Wall

BIN     := bin
SRC     := src
INCLUDE := include

all: client server

client: $(BIN)/dfc
server: $(BIN)/dfs

clean:
	rm -f $(BIN)/*

$(BIN)/dfc: $(SRC)/dfc.c $(SRC)/utils.c $(SRC)/socket.c #$(SRC)/str_utils.c $(SRC)/forwarding.c
	$(CC) $(CFLAGS) -I$(INCLUDE) $^ -o $@ -lcrypto -lssl -lz

$(BIN)/dfs: $(SRC)/dfs.c $(SRC)/utils.c $(SRC)/socket.c #$(SRC)/str_utils.c $(SRC)/forwarding.c
	$(CC) $(CFLAGS) -I$(INCLUDE) $^ -o $@ -lcrypto -lssl -lz
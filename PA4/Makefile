CC=gcc
CFLAGS=-g -Wall

BIN     := bin
SRC     := src
INCLUDE := include

all: client 

client: $(BIN)/dfc
# client: $(BIN)/udp_client

clean:
	rm -f $(BIN)/*

$(BIN)/dfc: $(SRC)/dfc.c $(SRC)/utils.c $(SRC)/socket.c #$(SRC)/str_utils.c $(SRC)/forwarding.c
	$(CC) $(CFLAGS) -I$(INCLUDE) $^ -o $@ -lcrypto -lssl -lz

# $(BIN)/udp_client: $(SRC)/udp_client.c $(SRC)/str_utils.c $(SRC)/file_transfer.c $(SRC)/utils.c
# 	$(CC) $(CFLAGS) -I$(INCLUDE) $^ -o $@
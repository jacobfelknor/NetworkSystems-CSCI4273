CC=gcc
CFLAGS=-g

BIN     := bin
SRC     := src
INCLUDE := include

all: proxy 

proxy: $(BIN)/proxy
# client: $(BIN)/udp_client

clean:
	rm -f $(BIN)/*

$(BIN)/proxy: $(SRC)/proxy.c $(SRC)/utils.c $(SRC)/str_utils.c $(SRC)/forwarding.c
	$(CC) $(CFLAGS) -I$(INCLUDE) $^ -o $@ -lcrypto -lssl -lz

# $(BIN)/udp_client: $(SRC)/udp_client.c $(SRC)/str_utils.c $(SRC)/file_transfer.c $(SRC)/utils.c
# 	$(CC) $(CFLAGS) -I$(INCLUDE) $^ -o $@
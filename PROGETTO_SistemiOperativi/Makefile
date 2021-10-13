# Set compiler flags I was asked for.
CFLAGS = -std=c89 -Wpedantic

# Set the name the application will be named by.
TARGET = game

# Add each object file required by the application.
OBJ = game.o library/board.o library/msg_manager.o library/pawn.o library/player.o

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -pthread -o $(TARGET)   

all: $(TARGET)

# Remove all object files.
clean:
	rm -f *.o library/*.o $(TARGET) *~

run: $(TARGET)
	./$(TARGET)

# $(LDFLAGS) specifica tutti i link necessari
# pthreads, sono un modello di esecuzione che esiste indipendentemente da un linguaggio di programmazione, nonché un modello di esecuzione parallelo. Consente a un programma di controllare più flussi di lavoro diversi che si sovrappongono nel tempo. Ogni flusso di lavoro viene definito thread e la creazione e il controllo di questi flussi si ottiene effettuando chiamate all'API dei thread POSIX. 
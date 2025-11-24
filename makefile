# Compilador a ser utilizado
CC = gcc

# Flags de compilação:
# -Wall -Wextra: Ativam a maioria dos avisos (warnings) úteis.
# -g: Adiciona informações de debug (útil para usar com gdb ou valgrind).
# -pthread: Essencial para compilar e linkar programas com threads POSIX.
CFLAGS = -Wall -Wextra -g -pthread

# Nome do arquivo executável final
TARGET = dining_hall

# Nome do arquivo fonte
SRC = dining_hall.c

# Regra padrão (o que acontece quando você digita apenas 'make')
all: $(TARGET)

# Regra para criar o executável
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Regra para limpar arquivos gerados (digite 'make clean')
clean:
	rm -f $(TARGET)

# Regra utilitária para rodar o programa com um valor padrão (digite 'make run')
run: $(TARGET)
	./$(TARGET) 10

# Regra para rodar com Valgrind (para verificar vazamento de memória/threads)
valgrind: $(TARGET)
	valgrind --tool=helgrind ./$(TARGET) 10

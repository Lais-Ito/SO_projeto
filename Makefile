# ============================================================================
# Makefile - Simulador Visual de Sistema Operacional (Projeto A)
# Compilação modular em C99 puro sem dependências externas.
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -g

SRCS = src/main.c src/parser.c src/escalonador.c src/engine.c src/frontend.c
OBJS = $(SRCS:.c=.o)
EXEC = simulador

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(EXEC) gantt_final.svg

test: $(EXEC)
	./$(EXEC) examples/config_exemplo.txt -c

.PHONY: all clean test

CC     = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = produtor_consumidor_v2
SRCS   = main.c buffer.c logger.c produtor.c consumidor.c
OBJS   = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

pdf:
	pdflatex -interaction=nonstopmode relatorio.tex
	pdflatex -interaction=nonstopmode relatorio.tex

clean:
	rm -f $(OBJS) $(TARGET) relatorio.aux relatorio.log relatorio.out relatorio.toc

.PHONY: all pdf clean

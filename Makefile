CC = /home/devel/arm-linux-2016.11/bin/arm-linux-gcc

cam4: cam4.c font_8x8.c font_8x16.c
	$(CC) -o $@ $^


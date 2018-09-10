CC = arm-linux-gcc

camera: camera.c font_8x8.c font_8x16.c framebuffer.c
	$(CC) -o $@ $^


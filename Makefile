CC = gcc


camera: camera.c font_8x8.c font_8x16.c
	$(CC) -o $@ $^


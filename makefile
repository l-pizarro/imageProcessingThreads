all:
	gcc -I/usr/local/Cellar/libpng/1.6.37/include/libpng16 -L/usr/local/Cellar/libpng/1.6.37/lib -lpng16 main.c functions/general/general.c functions/general/barrier_implementation.c functions/image_processing/image_processing.c -o lab2 -lm -Wall -lpng -lpthread

run:
	./lab2 -c 4 -m test.txt -n 40 -b -h 5 -t 15

clean:
	rm lab2
	rm output/*
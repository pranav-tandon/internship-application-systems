ping: ping.c
	gcc -g -Wall -o ping ping.c -lm

clean:
	rm ping
	rm -rf ping.dsYM

debug:
	make ping
  valgrind --leak-check=full ./ping

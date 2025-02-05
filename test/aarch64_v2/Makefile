all: main_host main_arm

main_host: main.c
	gcc -o main_host -Wall main.c

main_arm: main.c
	aarch64-linux-gnu-gcc -mcpu=cortex-a53 main.c -o main_arm

clean:
	rm -f main_host main_arm
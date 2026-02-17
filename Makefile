all: sim

OBJS = List.o PCB.o main.o deadlock.o

sim: $(OBJS)
	gcc $(OBJS) -o sim

List.o: List.c List.h
	gcc -c List.c

PCB.o: PCB.c PCB.h List.h deadlock.h
	gcc -c PCB.c

deadlock.o: deadlock.c deadlock.h PCB.h List.h
	gcc -c deadlock.c

main.o: main.c PCB.h List.h
	gcc -c main.c

clean:
	rm -f sim *.o
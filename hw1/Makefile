VISIT_INSTALL_DIR=/home/users/gravelle/visit2_12_3/2.12.3/linux-x86_64
LIBSIM_DIR=$(VISIT_INSTALL_DIR)/libsim/V2
LIBSIM_INCLUDE=-I$(LIBSIM_DIR)/include
LIBSIM_LIBS=-L$(LIBSIM_DIR)/lib -lsimV2
 
all: perlin

perlin: perlin.o
	g++ -g -o perlin.out perlin.o $(LIBSIM_LIBS) -ldl
 
perlin.o: perlin.c
	g++ $(LIBSIM_INCLUDE) -DCUR_PATH=\"$(PWD)\" -c -g perlin.c


clean:
	rm -f *.out *.o

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11
SOEM_DIR ?= $(HOME)/Tools/EtherCat/SOEM
SOEM_INC = -I$(SOEM_DIR)/include -I$(SOEM_DIR)/oshw/linux -I$(SOEM_DIR)/osal -I$(SOEM_DIR)/osal/linux
SOEM_LIBS = -L/usr/local/lib -lsoem -pthread -lm

TARGET = ecat_motor_rt
TEST_TARGET = test_motor_pdo

.PHONY: all test clean

all: $(TARGET)

$(TARGET): ecat_motor_rt.c motor_pdo.h

	$(CC) $(CFLAGS) $(SOEM_INC) -o $@ ecat_motor_rt.c $(SOEM_LIBS)

$(TEST_TARGET): test_motor_pdo.c motor_pdo.h

	$(CC) $(CFLAGS) -o $@ test_motor_pdo.c

test: $(TEST_TARGET)

	./$(TEST_TARGET)

clean:

	rm -f $(TARGET) $(TEST_TARGET)

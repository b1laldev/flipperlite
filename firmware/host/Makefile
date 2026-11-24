CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -g -I../main/flite -I../main/apps

SOURCES = \
	test_host.c \
	../main/flite/flite_protocol.c \
	../main/flite/flite_canvas.c \
	../main/flite/flite_scene.c \
	../main/flite/flite_manager.c \
	../main/apps/flite_apps.c

.PHONY: test clean

test: host_tests
	./host_tests

host_tests: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

clean:
	rm -f host_tests

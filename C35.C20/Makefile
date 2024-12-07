build: encrypt-driver.c encrypt-module.c encrypt-module.h circular-buffer.h reset-controller.h
	gcc encrypt-driver.c encrypt-module.c -lpthread -o encrypt

run: build
	./encrypt in.txt out.txt log.txt

start:
	./encrypt in.txt out.txt log.txt

# test: encrypt-driver-test.c encrypt-module-test.c encrypt-module.h circular-buffer.h reset-controller.h
# 	gcc encrypt-driver-test.c encrypt-module-test.c -lpthread -o encrypt-test

# run-test: test
# 	./encrypt-test encrypt-module.h cyper.txt

# start-test:
# 	./encrypt-test encrypt-module.h cyper.txt

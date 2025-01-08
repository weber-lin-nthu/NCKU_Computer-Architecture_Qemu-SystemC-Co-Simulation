#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define DevPath "/dev/calculator"

//
#define CALC_CTRL_GO		0x01
#define CALC_CTRL_ABORT		0x02
#define CALC_CTRL_CLRI		0x04
#define CALC_CTRL_RESET		0x08

#define CALC_OP_ADD			0x01
#define CALC_OP_SUB			0x02
#define CALC_OP_MUL			0x03
#define CALC_OP_DIV			0x04

#define CMD_STATUS			0x00
#define CMD_OP_SET			0x03
#define CMD_CTRL			0x04
//

static const unsigned int OP_LIST[4] = {
				CALC_OP_ADD,
				CALC_OP_SUB,
				CALC_OP_MUL,
				CALC_OP_DIV };

int main(int argc, char* argv[]) {
	int i;
	int buffer[2] = {0};
	int result[4] = {0};
	int calc_fd = 0;

	if(argc < 3) {
		printf("Usage: \n");
		printf("  %s <operand1> <operand2>\n", argv[0]);
		return 0;
	} else {
		buffer[0] = atoi(argv[1]);
		buffer[1] = atoi(argv[2]);
	}
	calc_fd = open(DevPath, O_RDWR);
	if(calc_fd == 0) {
		perror("Error opening device\n");
		return 1;
	}
	write(calc_fd, (void*)buffer, sizeof(unsigned int) * 2);
	for(i = 0; i < 4; i++) {
		ioctl(calc_fd, CMD_OP_SET, OP_LIST[i]);
		ioctl(calc_fd, CMD_CTRL, 0x01);
		read(calc_fd, (void*)&(result[i]), sizeof(int));
	}

	printf("%d + %d = %d\n", buffer[0], buffer[1], result[0]);
	printf("%d - %d = %d\n", buffer[0], buffer[1], result[1]);
	printf("%d * %d = %d\n", buffer[0], buffer[1], result[2]);
	printf("%d / %d = %d\n", buffer[0], buffer[1], result[3]);

	close(calc_fd);

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include "rxCommand.h"
#include "wrappers.h"

int main(void) {
	char *input = "grep really < catfile|sort`";

	int len, i, n;
	rxCommand **cmds;
	char *cmd, *out;
	len = rxCmd_Sub(input, &cmd, &i);
	if (len != -1) {
		printf("cmd: %s\n", cmd);
		
		printf("i: %d\n", i);
		printf("Rest of command: %s\n",input+i+1);
		cmds = rxParse(cmd);
		if (cmds) {
			n = rxCmd_ToString(cmds, &out);
			if (n >= 0) {
				printf("output: '%s'\n", out);
			}
		}
	}
	return 0;
}

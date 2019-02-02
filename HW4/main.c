#include <stdio.h>
#include <stdlib.h>

int main()
{
    while (1) {
	if (scanf("%s", buffer) == EOF) return 0;
	if (!isdigit(buffer[0])) {
        if (buffer[0] == 'p') {
            if (stack.peek() == NULL) {
                printf("dc: stack empty\n");
            }
            else {
			printf("%d\n", (int)stack.pop()); /* popl to some register */
			}
		} else if (buffer[0] == 'q') {
			goto quit;
		} else if (buffer[0] == '+') {
			int a, b;
			if (stack.peek() == NULL) {
				printf("dc: stack empty\n");
				continue;
			}
			a = (int)stack.pop();
			if (stack.peek() == NULL) {
				printf("dc: stack empty\n");
				stack.push(a); /* pushl some register value */
				continue;
			}
			b = (int)stack.pop(); /* popl to some register */
			res = a + b;
			stack.push(res);
		} else if (buffer[0] == '-') {
			/* ... */
		} else if (buffer[0] == '^') {
			/* ... powerfunc() ... */
		} else if { /* ... and so on ... */
	} else { /* the first no. is a digit */
		int no = atoi(buffer);
		stack.push(no);	/* pushl some register value */
	}
	}
}

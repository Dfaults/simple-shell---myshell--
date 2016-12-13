# 杨富强, 01051312 --- Operating Systems Project 1
#在Ubuntu 7.10 和cygwin 下都已经测试通过
myshell: myshell.c utility.c myshell.h
	gcc -Wall myshell.c utility.c -o myshell

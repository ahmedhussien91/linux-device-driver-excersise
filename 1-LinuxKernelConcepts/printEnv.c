extern char **environ;
int main(int argc, char *argv[])
{
char **ep;
for (ep = environ; *ep != 0; ep++)
	puts(*ep);
exit(0);
}

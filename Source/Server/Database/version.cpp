#include <cstdio>
#include <version.h>

void WriteVersion()
{
	FILE* fp = fopen("VERSION.txt", "w");

	if (fp)
	{
		fprintf(fp, "YitServer Revision: %s\n\n", __REVISION__);
		fprintf(fp, "Compilation Info:\n");
		fprintf(fp, "\tSystem Name: %s\n", __OS_NAME__);
		fprintf(fp, "\tCompiler Name: %s\n", __COMPILER_NAME__);
		fprintf(fp, "\tTarget Architecture: %s\n", __CPU_TARGET__);
		fclose(fp);
	}
}


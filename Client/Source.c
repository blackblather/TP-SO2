#include <stdio.h>
#include <windows.h>
#include "dll.h"

//N�o vale a pena carregar a DLL de forma din�mica, porque o Client � constru�do 100% com base na DLL
int main(int argc, const char* argv[]) {
	int swag = InitializeSwag();
	return 0;
}
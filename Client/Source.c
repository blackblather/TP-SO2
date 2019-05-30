#include <stdio.h>
#include <windows.h>
#include "dll.h"

//Não vale a pena carregar a DLL de forma dinâmica, porque o Client é construído 100% com base na DLL
int main(int argc, const char* argv[]) {
	int swag = InitializeSwag();
	return 0;
}
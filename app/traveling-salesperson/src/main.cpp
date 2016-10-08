#include "ionlib\log.h"
#include "ionlib\net.h"

int main(int argc, char* argv[])
{
	ion::InitSockets();
	ion::LogInit("app");
	return 0;
}

#include "alloc.h"
int main()
{
	MemoryAllocator ma;
	ma.init();
	void* p1 = ma.alloc(600);
	void* p2 = ma.alloc(900);
	void* p3 = ma.alloc(1400);
	ma.dumpBlocks();
	ma.free(p1);
	ma.dumpBlocks();
	ma.free(p2);
	ma.dumpBlocks();
	ma.free(p3);
	ma.dumpBlocks();
	ma.destroy();
}
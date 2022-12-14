#include "alloc.h"

MemoryAllocator::MemoryAllocator() { }
MemoryAllocator::~MemoryAllocator()
{
	assert(service.wasDestroyCalled == service.wasInitCalled);
}
void MemoryAllocator::init()
{
	fsAlloc16.init(16);
	fsAlloc32.init(32);
	fsAlloc64.init(64);
	fsAlloc128.init(128);
	fsAlloc256.init(256);
	fsAlloc512.init(512);
	cAlloc.init();
	fAlloc.init();
	service.wasInitCalled = true;
}
void MemoryAllocator::destroy()
{
	assert(service.wasInitCalled == true);
	assert(isFree() == true);
	fsAlloc16.destroy();
	fsAlloc32.destroy();
	fsAlloc64.destroy();
	fsAlloc128.destroy();
	fsAlloc256.destroy();
	fsAlloc512.destroy();
	cAlloc.destroy();
	fAlloc.destroy();
	service.wasDestroyCalled = true;
}
void* MemoryAllocator::alloc(size_t size)
{
	assert(service.wasInitCalled == true);
	if (size <= 16)
		return fsAlloc16.alloc();
	if (size <= 32)
		return fsAlloc32.alloc();
	if (size <= 64)
		return fsAlloc64.alloc();
	if (size <= 128)
		return fsAlloc128.alloc();
	if (size <= 256)
		return fsAlloc256.alloc();
	if (size <= 512)
		return fsAlloc512.alloc();
	if (size <= 10485760)
		return cAlloc.alloc(size);
	return fAlloc.alloc(size);
}
void MemoryAllocator::free(void* p)
{
	assert(service.wasInitCalled == true);
	bool freedSuccessfully = fsAlloc16.free(p);
	if (freedSuccessfully == true) 
		goto X;
	freedSuccessfully = fsAlloc32.free(p);
	if (freedSuccessfully == true)
		goto X;
	freedSuccessfully = fsAlloc64.free(p);
	if (freedSuccessfully == true)
		goto X;
	freedSuccessfully = fsAlloc128.free(p);
	if (freedSuccessfully == true)
		goto X;
	freedSuccessfully = fsAlloc256.free(p);
	if (freedSuccessfully == true)
		goto X;
	freedSuccessfully = fsAlloc512.free(p);
	if (freedSuccessfully == true)
		goto X;
	freedSuccessfully = cAlloc.free(p);
	if (freedSuccessfully == true)
		goto X;
	freedSuccessfully = fAlloc.free(p);
X:	assert(freedSuccessfully == true);
	
}
void MemoryAllocator::dumpStat() const
{
	assert(service.wasInitCalled == true);
	std::cout << "MEMORY ALLOCATOR STATS\n";
	std::cout << "Total memory requested from the OS: " << 
		fsAlloc16.getAllocatedSize() +
		fsAlloc32.getAllocatedSize() +
		fsAlloc64.getAllocatedSize() +
		fsAlloc128.getAllocatedSize() +
		fsAlloc256.getAllocatedSize() +
		fsAlloc512.getAllocatedSize() +
		cAlloc.getAllocatedSize() +
		fAlloc.getOccupiedSize() << " bytes.\n";
	std::cout << "Total memory occupied: " <<
		fsAlloc16.getOccupiedSize() +
		fsAlloc32.getOccupiedSize() +
		fsAlloc64.getOccupiedSize() +
		fsAlloc128.getOccupiedSize() +
		fsAlloc256.getOccupiedSize() +
		fsAlloc512.getOccupiedSize() +
		cAlloc.getOccupiedSize() +
		fAlloc.getOccupiedSize() << " bytes.\n";
	fsAlloc16.dumpStat();
	fsAlloc32.dumpStat();
	fsAlloc64.dumpStat();
	fsAlloc128.dumpStat();
	fsAlloc256.dumpStat();
	fsAlloc512.dumpStat();
	cAlloc.dumpStat();
	fAlloc.dumpStat();
	std::cout << "\n\n";
}
void MemoryAllocator::dumpBlocks() const
{
	fsAlloc16.dumpBlocks();
	fsAlloc32.dumpBlocks();
	fsAlloc64.dumpBlocks();
	fsAlloc128.dumpBlocks();
	fsAlloc256.dumpBlocks();
	fsAlloc512.dumpBlocks();
	cAlloc.dumpBlocks();
	/*fAlloc.dumpBlocks();*/
	std::cout << "\n";
}
bool MemoryAllocator::isFree() const
{
	return fsAlloc16.isFree() &&
		fsAlloc32.isFree() &&
		fsAlloc64.isFree() &&
		fsAlloc128.isFree() &&
		fsAlloc256.isFree() &&
		fsAlloc512.isFree() &&
		cAlloc.isFree() &&
		fAlloc.isFree();
}
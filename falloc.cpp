#include "alloc.h"

FallbackAlloc::FallbackAlloc() : first(NULL), service(Service()) {};
FallbackAlloc::~FallbackAlloc() {};
void FallbackAlloc::init() {};
void FallbackAlloc::destroy() 
{
	assert(isFree() == true);
};
void* FallbackAlloc::alloc(size_t size)
{
	FAPage* newPage = new FAPage;
	newPage->pageStart = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	newPage->pageSize = size;
	if (first == NULL)
		first = newPage;
	else
	{
		FAPage* anotherPage = first;
		while (anotherPage->nextPage != NULL)
			anotherPage = anotherPage->nextPage;
		anotherPage->nextPage = newPage;
	}
	service.occupiedSize += size;
	return newPage->pageStart;
}
bool FallbackAlloc::free(void* p)
{
	bool freedSuccessfully = false;
	FAPage* toFree = NULL;
	if (p == first->pageStart)
	{
		toFree = first;
		first = first->nextPage;
		freedSuccessfully = true;
	}
	else
		for (FAPage* anotherPage = first; anotherPage->nextPage != NULL; anotherPage = anotherPage->nextPage)
			if (anotherPage->nextPage == p)
			{
				toFree = anotherPage->nextPage;
				anotherPage->nextPage = anotherPage->nextPage->nextPage;
				freedSuccessfully = true;
				service.allocPages--;
				break;
			}
	if (freedSuccessfully == true)
	{
		service.occupiedSize -= toFree->pageSize;
		VirtualFree(p, 0, MEM_RELEASE);
		delete toFree;	
	}
	return freedSuccessfully;
}
//void FallbackAlloc::dumpBlocks() const
//{
//	int i = 0;
//	for (FAPage const* anotherPage = first; anotherPage != NULL; anotherPage = anotherPage->nextPage)
//	{
//		std::cout << "PAGE #" << i << ": SIZE = " << anotherPage->pageSize << '\n';
//		i++;
//	}
//	std::cout << '\n';
//}
bool FallbackAlloc::isFree() const
{
	return service.allocPages == 0;
}

size_t FallbackAlloc::getOccupiedSize() const
{
	return service.occupiedSize;
}

void FallbackAlloc::dumpStat() const
{
	std::cout << "FALLBACK ALLOCATOR STATS\n";
	int i = 0;
	std::cout << "\tPAGE#" << "\tSIZE\n";
	for (FAPage const* anotherPage = first; anotherPage != NULL; anotherPage = anotherPage->nextPage)
		std::cout << '\t' << i++ << '\t' << anotherPage->pageSize << '\n';
}
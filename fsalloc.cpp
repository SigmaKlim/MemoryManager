#include "alloc.h"

FSAPage::FSAPage()
{
	pageStart = NULL;
	nextPage = NULL;
	fH = NULL;
	numLinked = 0;
	blockSize = 0;
	numBlocks = 0;
	metaSizeRounded = sizeof(Metadata) + sizeof(Metadata) % 8;
};
FSAPage::~FSAPage() {};
void FSAPage::init(size_t size)
{
	pageStart = VirtualAlloc(NULL, PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	blockSize = size + size % 8 + metaSizeRounded;
	numBlocks = PAGE_SIZE / blockSize;
	service.numPages++;
}
void FSAPage::destroy()
{
	assert(isFree() == true);
	for (FSAPage* toDestroy = this; toDestroy != NULL; toDestroy = toDestroy->nextPage)
	{
		bool destroyedSuccessfully = VirtualFree(toDestroy->pageStart, 0, MEM_RELEASE);
		assert(destroyedSuccessfully == true);
	}
}
void* FSAPage::alloc()
{
	service.numAlloc++;
	for (FSAPage* anotherPage = this; anotherPage != NULL; anotherPage = anotherPage->nextPage)
		if (anotherPage->numLinked < numBlocks)
		{
			void* newBlock = new((byte*)anotherPage->pageStart + anotherPage->numLinked * blockSize) Metadata;
			anotherPage->numLinked++;
			return (byte*)newBlock + metaSizeRounded;
		}
	FSAPage* anotherPage = this;
	while (true)
	{
		if (anotherPage->fH != NULL)
		{
			void* newBlock = anotherPage->fH;
			if (((Metadata*)newBlock)->freeDist == 0)
				anotherPage->fH = NULL;
			else
				anotherPage->fH = (byte*)anotherPage->fH + ((Metadata*)newBlock)->freeDist * blockSize;
			new (newBlock) Metadata; //!!!
			return (byte*)newBlock + metaSizeRounded;
		}
		if (anotherPage->nextPage == NULL)
			break;
		anotherPage = anotherPage->nextPage;
	}
	FSAPage* newPage = new FSAPage;
	service.numPages++;
	newPage->init(blockSize - metaSizeRounded);
	newPage->numLinked++;
	anotherPage->nextPage = newPage;
	void* newBlock = new((byte*)newPage->pageStart) Metadata;
	return (byte*)newBlock + metaSizeRounded;
}
bool FSAPage::free(void* p)
{
	bool freedSuccessfully = false;
	for (FSAPage* anotherPage = this; anotherPage != NULL; anotherPage = anotherPage->nextPage)
		if ((byte*)p - (byte*)anotherPage->pageStart >= 0 && (byte*)p - (byte*)anotherPage->pageStart < PAGE_SIZE)
		{
			void* toFree = (byte*)p - metaSizeRounded;
			assert(((Metadata*)toFree)->marker == INT_MAX); //check the integrity of the memory block to free
			assert(((Metadata*)toFree)->isFree == false); //check that the block has not yet been freed
			service.numAlloc--;
			/*if (anotherPage->fH == NULL)
				((Metadata*)toFree)->freeDist = 0;
			else*/
			if (anotherPage->fH != NULL) //!!!
				((Metadata*)toFree)->freeDist = (byte*)(anotherPage->fH) - (byte*)toFree;
			anotherPage->fH = toFree;
			((Metadata*)toFree)->isFree = true;
			freedSuccessfully = true;
			break;
		}
	return freedSuccessfully;
}
void FSAPage::dumpBlocks() const
{
	std::cout << "FSA" << blockSize - metaSizeRounded << "  BLOCK STATS\n";
	std::cout << "BLOCKS ALLOCATED: " << service.numAlloc << '\n';
	int i = 0;
	for (FSAPage const* anotherPage = this; anotherPage != NULL; anotherPage = anotherPage->nextPage)
	{
		std::cout << "PAGE #" << i << '\n';
		std::cout << "\t#\t \tAddress\t \tSize\t \tIsFree?\n";
		void* p = anotherPage->pageStart;
		for (int j = 0; j < anotherPage->numLinked; j++)
		{
			std::cout << "\t" << j << "\t \t" << p << "\t" << blockSize << "\t \t" << ((Metadata*)p)->isFree << '\n';
			p = (byte*)p + blockSize;
		}
		std::cout << "\t" << anotherPage->numLinked << "\t \t" << (void*)((byte*)p + blockSize) << "\t" << PAGE_SIZE - anotherPage->numLinked * blockSize << "\t1\n";
		i++;
	}
	std::cout << '\n';
}
void FSAPage::dumpStat() const
{
	std::cout << "FSA" << blockSize - metaSizeRounded<< " STATS\n";
	int i = 0;
	std::cout << "\tPAGE#" << "\tSIZE\n" ;
	for (FSAPage const* anotherPage = this; anotherPage != NULL; anotherPage = anotherPage->nextPage)
		std::cout << '\t' << i++ << '\t' << PAGE_SIZE <<'\n';
}
bool FSAPage::isFree() const
{
	return service.numAlloc == 0;
}
size_t FSAPage::getOccupiedSize() const
{
	return service.numAlloc * blockSize;
}
size_t FSAPage::getAllocatedSize() const
{
	return service.numPages * PAGE_SIZE;
}

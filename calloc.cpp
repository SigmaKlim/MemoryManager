#include "alloc.h"

CAPage::CAPage()
{
	pageStart = NULL;
	nextPage = NULL;
	fH = NULL;
	numBlocks = 0;
	metaSizeRounded = sizeof(Metadata) + sizeof(Metadata) % 8;
}
CAPage::~CAPage() {};
void CAPage::init()
{
	pageStart = VirtualAlloc(NULL, PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	service.numPages++;
	fH = pageStart;
	new(pageStart) Metadata;
	((Metadata*)pageStart)->blockSize = PAGE_SIZE;
	((Metadata*)pageStart)->isFree = true;
}
void CAPage::destroy()
{
	assert(isFree() == true);
	for (CAPage* toDestroy = this; toDestroy != NULL; toDestroy = toDestroy->nextPage)
	{
		bool freedSuccessfully = VirtualFree(toDestroy->pageStart, 0, MEM_RELEASE);
		assert(freedSuccessfully == true);
	}
}
void* CAPage::alloc(size_t size)
{
	CAPage* anotherPage = this;
	void* newBlock = NULL;
	size_t newBlockSize = size + size % 8 + metaSizeRounded; //required size of a new block including metadata size
	assert(PAGE_SIZE >= newBlockSize);
	while (true) //iterate through pages
	{
		newBlock = anotherPage->fH;
		while (newBlock != NULL) //iterate through free-list
			if (((Metadata*)newBlock)->blockSize >= newBlockSize) //check if another block from free-list is large enought to be allocated, if so - exit all loops
				goto X;
			else
				if (((Metadata*)newBlock)->nextFreeDist == 0) //check if there is more in free-list
					break;
				else
					newBlock = (byte*)newBlock + ((Metadata*)newBlock)->nextFreeDist;
		if (anotherPage->nextPage == NULL) //check if the next page exists, if not ask for a new page
		{
			CAPage* newPage = new CAPage;
			service.numPages++;
			newPage->init();
			anotherPage->nextPage = newPage;
		}
		anotherPage = anotherPage->nextPage;
	}
X:
	assert(((Metadata*)newBlock)->marker == INT_MAX); //check the found block for memory corruption
	((Metadata*)newBlock)->isFree = false;
	((Metadata*)newBlock)->nextFreeDist = 0;
	((Metadata*)newBlock)->prevFreeDist = 0;
	if (((Metadata*)newBlock)->nextFreeDist == 0)
		anotherPage->fH = NULL;
	else
		anotherPage->fH = (byte*)newBlock + ((Metadata*)newBlock)->nextFreeDist;
	if (((Metadata*)newBlock)->blockSize > newBlockSize + metaSizeRounded) //check if the new block is large enough to potentially be allocated from in future; if so - create extra block after the new block
	{
		void* extraBlock = (byte*)newBlock + newBlockSize;
		new(extraBlock) Metadata;
		((Metadata*)(extraBlock))->isFree = true;
		((Metadata*)(extraBlock))->blockSize = ((Metadata*)newBlock)->blockSize - newBlockSize;
		((Metadata*)(extraBlock))->nextFreeDist = (byte*)(anotherPage->fH) - extraBlock;
		((Metadata*)(extraBlock))->prevBlockDist = (-1) * newBlockSize;
		anotherPage->fH = extraBlock;
		((Metadata*)newBlock)->blockSize = newBlockSize;
		if ((byte*)extraBlock + ((Metadata*)extraBlock)->blockSize - anotherPage->pageStart < PAGE_SIZE)
			((Metadata*)((byte*)extraBlock) + ((Metadata*)extraBlock)->blockSize)->prevBlockDist = ((Metadata*)extraBlock)->blockSize;
		service.occupiedSize += newBlockSize;
	}
	else
		service.occupiedSize += ((Metadata*)newBlock)->blockSize;
	void* nextBlock = (byte*)newBlock + ((Metadata*)newBlock)->nextFreeDist;
	void* prevBlock = (byte*)newBlock + ((Metadata*)newBlock)->prevFreeDist;
	((Metadata*)prevBlock)->nextFreeDist = (byte*)nextBlock - (byte*)prevBlock; //repair free-list
	service.numAlloc++;
	return (byte*)newBlock + metaSizeRounded;
}
bool CAPage::free(void *p)
{
	bool freedSuccessfully = false;
	void* toFree = (byte*)p - metaSizeRounded;
	for (CAPage* anotherPage = this; anotherPage != NULL; anotherPage = anotherPage->nextPage) //iterate through pages
		if ((byte*)p - (byte*)anotherPage->pageStart >= 0 && (byte*)p - (byte*)anotherPage->pageStart < PAGE_SIZE) //check if given pointer belongs to a page 
		{
			assert(((Metadata*)toFree)->marker == INT_MAX); //check the integrity of the memory block to free
			assert(((Metadata*)toFree)->isFree == false); //check that the block has not yet been freed
			void* nextBlock = (byte*)toFree + ((Metadata*)toFree)->blockSize;
			void* prevBlock = (byte*)toFree + ((Metadata*)toFree)->prevBlockDist;
			bool nextExists = false, prevExists = false;
			if ((byte*)nextBlock - (byte*)anotherPage->pageStart < PAGE_SIZE)
			{
				nextExists = true;
				assert(((Metadata*)nextBlock)->marker == INT_MAX);
			}
			if ((byte*)prevBlock - (byte*)anotherPage->pageStart >= 0)
			{
				prevExists = true;
				assert(((Metadata*)prevBlock)->marker == INT_MAX);
			}
			((Metadata*)toFree)->isFree = true;
			if (prevExists == true && ((Metadata*)prevBlock)->isFree == true)
			{
				((Metadata*)prevBlock)->blockSize += ((Metadata*)toFree)->blockSize;
				toFree = prevBlock;
				((Metadata*)nextBlock)->prevBlockDist = (-1) * ((Metadata*)toFree)->blockSize;
			}
			if (nextExists == true && ((Metadata*)nextBlock)->isFree == true)
			{
				if (nextBlock == anotherPage->fH)
					anotherPage->fH = toFree;
				((Metadata*)toFree)->blockSize += ((Metadata*)nextBlock)->blockSize;
				((Metadata*)((byte*)nextBlock + ((Metadata*)nextBlock)->prevFreeDist))->nextFreeDist = ((Metadata*)nextBlock)->nextFreeDist - ((Metadata*)nextBlock)->prevFreeDist;
				if (((byte*)nextBlock + ((Metadata*)nextBlock)->blockSize) - anotherPage->pageStart < PAGE_SIZE)
					((Metadata*)((byte*)nextBlock + ((Metadata*)nextBlock)->blockSize))->prevBlockDist = ((Metadata*)toFree)->blockSize;
			}
			if ((prevExists == false || ((Metadata*)prevBlock)->isFree == false) && (nextExists == false || ((Metadata*)nextBlock)->isFree == false))
			{
				((Metadata*)toFree)->nextFreeDist = (byte*)(anotherPage->fH) - (byte*)toFree;
				anotherPage->fH = toFree;
			}
			freedSuccessfully = true;
			break;
		}
	if (freedSuccessfully == true)
	{
		service.numAlloc--;
		service.occupiedSize -= ((Metadata*)toFree)->blockSize;
	}
	return freedSuccessfully;
}
void CAPage::dumpBlocks() const
{
	std::cout << "CA BLOCK STATS\n";
	std::cout << "BLOCKS ALLOCATED: " << service.numAlloc << '\n';
	int i = 0;
	for (CAPage const* anotherPage = this; anotherPage != NULL; anotherPage = anotherPage->nextPage)
	{
		std::cout << "PAGE #" << i << '\n';
		int j = 0;
		std::cout << "\t#\t\tAddress\t\tSize\t\tIsFree?\n";
		for (void* anotherBlock = anotherPage->pageStart; (byte*)anotherBlock - ((byte*)anotherPage->pageStart) < PAGE_SIZE; anotherBlock = (byte*)anotherBlock + ((Metadata*)anotherBlock)->blockSize)
		{
			std::cout << "\t" << j << "\t\t" << anotherBlock << "\t" << ((Metadata*)anotherBlock)->blockSize << "\t\t" << ((Metadata*)anotherBlock)->isFree << '\n';
			j++;
		}
		i++;
	}
	std::cout << '\n';
}
bool CAPage::isFree() const
{
	return service.numAlloc == 0;
};
size_t CAPage::getOccupiedSize() const
{
	return service.occupiedSize;
}
size_t CAPage::getAllocatedSize() const
{
	return service.numPages * PAGE_SIZE;
}

void CAPage::dumpStat() const
{
	std::cout << "CA STATS\n";
	int i = 0;
	std::cout << "\tPAGE#" << "\tSIZE\n";
	for (CAPage const* anotherPage = this; anotherPage != NULL; anotherPage = anotherPage->nextPage)
		std::cout << '\t' << i++ << '\t' << PAGE_SIZE << '\n';
}
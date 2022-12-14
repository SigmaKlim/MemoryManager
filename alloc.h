#pragma once
#include <Windows.h>
#include <cassert>
#include <string>
#include <iostream>
#include <stdio.h>

const size_t PAGE_SIZE = 10485760;

class FSAPage //fixed-size allocator
{
public:
	struct Metadata
	{
		bool isFree = false;
		const int marker = INT_MAX;
		ptrdiff_t freeDist = 0; //distance towards the next free memory block
	};
	struct Service
	{
		/*bool wasDestroyCalled = false;*/
		size_t numPages = 0;
		size_t numAlloc = 0; //the number of allocated blocks
	};
	FSAPage();
	~FSAPage();
	void init(size_t size);
	void destroy();
	void* alloc();
	bool free(void* p);
	void dumpBlocks() const;
	void dumpStat() const;

	size_t getOccupiedSize() const;
	size_t getAllocatedSize() const;
	bool isFree() const;
private:
	void* pageStart;
	FSAPage* nextPage;
	void* fH; //free-list head ptr
	size_t numLinked; //number of linked blocks on the page
	size_t numBlocks; //total number of blocks on the page

	size_t blockSize; //size of a block (incl. metadata size)
	size_t metaSizeRounded; //size of metadata struct rounded to the next multiple of 8
	Service service;
};

class CAPage //coalesce allocator
{
public:
	struct Metadata
	{
		bool isFree = false;
		const int marker = INT_MAX;
		ptrdiff_t nextFreeDist = 0; //distance towards the next memory block
		ptrdiff_t prevFreeDist = 0; //distance towards the previous memory block
		size_t blockSize = 0;
		ptrdiff_t prevBlockDist = -1000; //previous (on the page) block size (used to access its metadata)
	};
	struct Service
	{
		/*bool wasDestroyCalled = false;*/
		size_t numPages = 0;
		size_t numAlloc = 0; //the number of allocated blocks
		size_t occupiedSize = 0;
	};
	CAPage();
	~CAPage();
	void init();
	void destroy();
	void* alloc(size_t size);
	bool free(void* p);
	void dumpBlocks() const;
	void dumpStat() const;
	size_t getOccupiedSize() const;
	size_t getAllocatedSize() const;
	bool isFree() const;
private:
	void* pageStart;
	CAPage* nextPage;
	void* fH; //free-list head ptr
	size_t numBlocks;
	size_t metaSizeRounded;

	Service service;
};

class FallbackAlloc
{
	struct Service
	{
		size_t allocPages = 0;
		size_t occupiedSize = 0;
	};
	struct FAPage
	{
		void* pageStart = NULL;
		FAPage* nextPage = NULL;
		FAPage* prevPage = NULL;
		size_t pageSize = 0;
	};
public:
	FallbackAlloc();
	virtual ~FallbackAlloc();
	void init();
	void destroy();
	void* alloc(size_t size);
	bool free(void* p);
	/*void dumpBlocks() const;*/
	void dumpStat() const;

	size_t getOccupiedSize() const;
	bool isFree() const;
private:
	FAPage* first;
	Service service;
};

class MemoryAllocator 
{
	struct Service
	{
		bool wasInitCalled = false;
		bool wasDestroyCalled = false;
	};
public:
	MemoryAllocator();
	virtual ~MemoryAllocator();
	virtual void init();
	virtual void destroy();
	virtual void* alloc(size_t size);
	virtual void free(void* p);
	virtual void dumpStat() const;
	virtual void dumpBlocks() const;
	bool isFree() const;
private:
	FSAPage fsAlloc16;
	FSAPage fsAlloc32;
	FSAPage fsAlloc64;
	FSAPage fsAlloc128;
	FSAPage fsAlloc256;
	FSAPage fsAlloc512;
	CAPage cAlloc;
	FallbackAlloc fAlloc;

	Service service;
};




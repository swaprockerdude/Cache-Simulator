#include <iostream>
#include "cachesim.h"
#include<cmath>
#include<cstdlib>

using namespace std;

const int LINESIZE = 4;     // line size in bytes, must be power of 2, min=4 !!
const int SIZE     = 8192;  // total capacity in bytes, must be power of 2
const int WB_DEPTH = 1;     // depth of Wr buffer, 0 means none

// You shouldn't need to change any of the below constants

const int LINES    = SIZE/LINESIZE; // number of lines in the cache
const int WPLINE   = LINESIZE/4;    // words per line

// ------------------------------------------------------------------------
// Memory accesses take place as follows:
//   1 cycle to send address
//   10 cycles RAM latency to get first word
//   1 cycle to send each word
//   words in successive memory locations can be supplied at shorter
//   latency of 3 cycles
// Reading 1 word  = 1 + 10 + 1
// Reading n successive words = as above + (3+1)*(n-1)
// We assume writes don't generally occur to successive locations,
// so we must pay the full penalty for each write...
// ------------------------------------------------------------------------

const int SEND_LINES         = 1;
const int DRAM_LATENCY       = 10;
const int DRAM_LATENCY_NEXT  = 3;
const int SEND_WORD          = 1;
const int RM_PENALTY         = SEND_LINES + DRAM_LATENCY + SEND_WORD + 
                               ((WPLINE-1) * (DRAM_LATENCY_NEXT+SEND_WORD));
const int WM_PENALTY         = SEND_LINES + DRAM_LATENCY + SEND_WORD;

int flag;

// ------------------------------------------------------------------------
// Below are two important global functions which, given an address,
// return the cache index or tag.
// You need to fill in the details for these two functions:
// ------------------------------------------------------------------------

int INDEX(int addr) { 

 int binv[32] = {0},i = 0, j = 0, tag = 0, ind = 0, off = 0; 
if(addr != 0)
 {
   while(addr != 1)
   {
    binv[i] = addr%2; //calc bin
    addr = addr/2;
    i++;
   }
   binv[i] = 1;
 }
 else
 {
  binv[i] = 0;
 }

  //-------------------------------------------------------
  // Calculate tag bits and its int val
  //-------------------------------------------------------
   /*cout << endl;
   cout << "No. of tag bits = 19 : ";*/
   j = 31;  
   while(j > 12)
   {
    //cout << binv[j];
    tag += (pow(2,j-12-1))*(binv[j]);
    j--;
   }

//cout << endl << "tag val in int " << tag<< endl;

 //-------------------------------------------------------
 // Print index bits and its int val
 //-------------------------------------------------------
   //cout << "No. of index bits = 11 : ";
  
  while(j > 1)
   {
    //cout << binv[j]; 
    ind += (pow(2,j-1-1))*(binv[j]);
    j--; 
   }

 //cout << endl << "index val in int " << ind << endl;

//***********Print offset bits and its int val************
   //cout << "No. of offset bits = 2 : ";
  
  while(j >= 0)
   {
    //cout << binv[j]; 
    off += (pow(2,j))*(binv[j]);
    j--; 
   }
   //cout << endl << "off val in int " << off << endl;
 return ind; 
}


int TAG(int addr) { 

 int binv[32] = {0}, i = 0, j = 0, tag = 0, ind = 0, off = 0; 
if(addr != 0)
 {
   while(addr != 1)
   {
    binv[i] = addr%2; //calc bin
    addr = addr/2;
    i++;
   }
   binv[i] = 1;
 }
 else
 {
  binv[i] = 0;
 }

	//-------------------------------------------------------
  // Print tag bits and its int val
	//-------------------------------------------------------
   /*cout << endl;
   cout << "No. of tag bits = 19 : ";*/
  j = 31;  
   while(j > 12)
   {
    //cout << binv[j];
    tag += (pow(2,j-12-1))*(binv[j]);
    j--; 
   }

//cout << endl << "tag val in int " << tag<< endl;

 //-------------------------------------------------------
 // Print index bits and its int val
 //-------------------------------------------------------
   //cout << "No. of index bits = 11 : ";
  
  while(j > 1)
   {
    //cout << binv[j]; 
    ind += (pow(2,j-1-1))*(binv[j]);
    j--; 
   }

 //cout << endl << "index val in int " << ind << endl;

 //-------------------------------------------------------
 // Print offset bits and its int val
 //-------------------------------------------------------
   //cout << "No. of offset bits = 2 : ";
  
  while(j >= 0)
   {
    //cout << binv[j]; 
    off += (pow(2,j))*(binv[j]);
    j--; 
   }
   //cout << endl << "off val in int " << off << endl;

return tag; 
}


// ------------------------------------------------------------------------
// Below is the code for implementing write buffers.  You shouldn't
// need to study it unless you're doing extra credit. 
// ------------------------------------------------------------------------

int 
WriteBuffer::relinquishBus() //direct call from rd miss
{
  if (maxCycles == 0)  // no write buffer
    return 0;
  else {
    int temp = cyclesUntilEmpty % memLatency; //gives number of cycles we waited before we got the bus to fulfil rd miss OR num of cycles left to finish the current write to mem
    cyclesUntilEmpty -= temp;
    return temp;
  }
}
int 
WriteBuffer::addItem(int addr)  // addr is currently not used.  Direct call from wr hit/miss
{
  if (maxCycles == 0) // no write buffer
    return memLatency;  // always pay the full penalty
  else {
    int cycles = 0;
    if (isFull())
      cycles = relinquishBus(); //call relinq bus to get num of cycles we waited to add item to wb
    cyclesUntilEmpty += memLatency; //add the item
    return cycles;
  }
}
int 
WriteBuffer::advanceCycles(int cycles)  //direct call from rd hit
{ 
  if (maxCycles > 0) {
    int temp = cyclesUntilEmpty;
    if (cyclesUntilEmpty < cycles)
      cyclesUntilEmpty = 0;
    else {
      cyclesUntilEmpty = cyclesUntilEmpty - cycles;  //cycles mostly hold 1
      temp = cycles;
    }
    return temp;  // how many cycles did we actually consume?
  }
  return 0;
}

WriteBuffer::WriteBuffer(int size, int cyclesPerItem) 
{
  cout << "wb_size = " << size << endl;
  memLatency = cyclesPerItem; //wr miss penalty = cyclesPerItem
  cyclesUntilEmpty = 0;
  maxCycles = size * memLatency; 
}
  

// ------------------------------------------------------------------------
// Below is the implementation of CacheBlock's methods.
// The interesting ones are read and write.
// ------------------------------------------------------------------------

CacheBlock::CacheBlock() 
{
  valid = 0;
}

//------------------------------------------------------------------------
// Get the ref count for a blk
// ------------------------------------------------------------------------
int CacheBlock::getrefcount()
{
 return refCount;
}

// ------------------------------------------------------------------------
// For a WriteThrough cache:
//  Readhits:  we can provide the data directly, so no stall occurs,
//     and we give the writebuffer one cycle.
//  Readmisses: we need to fetch the data from memory.  This means first
//     contending for the bus with the writebuffer, and then actually 
//     fetching the data from memory, setting the line to valid, and updating
//     the tag.
// Currently implements no write-buffer.  You need to add the right lines
// of code to use the write-buffer.  Later, you'll modify this method to
// implement a write-back policy.
// ------------------------------------------------------------------------

int 
CacheBlock::read(int addr, WriteBuffer& writeBuffer, int& cycles) 
{
  if (valid && tag == TAG(addr)) {  // read hit 
    writeBuffer.advanceCycles(1); //give one cycle to wb

    cycles = 0;  // don't stall on a read hit
    cycles = cycles + 8;
    refCount--; //count number of times a block was referred
    if(refCount == 0)  //check if count for block is reached for referring it 3 times consecutively
    {
     flag = 1;
    }
    return 1;
  }
  else {         // read miss
    int wait_time = writeBuffer.relinquishBus(); //ask wb to relinquish the bus
    cycles = wait_time + RM_PENALTY;   // now fetch item from memory
    valid = 1;
    tag = TAG(addr);
    refCount = 50;
    return 0;
  }
}

// ------------------------------------------------------------------------
// For a WriteThrough cache:
//  Writehits:  we always update the next level of our hierarchy, so we
//     need to stick the word we're writing into the writebuffer
//  WriteMisses: since we're modeling a the writearound policy, we don't
//     make any changes to the cache, but just write to the next level
//     of our hierarchy, which again means giving the word to the writebuffer.
// Currently implements no write-buffer.  You need to add the right lines
// of code to use the write-buffer.  Later, you'll modify this method to
// implement a write-back policy.
// ------------------------------------------------------------------------

int 
CacheBlock::write(int addr, WriteBuffer& writeBuffer, int& cycles) 
{
  // With WT, on a hit or miss, we always write around to memory...
  
  if(valid && tag == TAG(addr)){ //wr hit
    cycles = writeBuffer.addItem(addr); //chk if we need to wait before adding the item to wb
    cycles = cycles + 8;
    return 1;
  }
  else{ //wr miss
    valid = 1; tag = TAG(addr);
    cycles = writeBuffer.addItem(addr); //chk if we need to wait before adding the item to wb
    return 0;
  }
  //cycles = WM_PENALTY; //Now that the writebuffer is taking care of writing the item, we no longer need to pay the memory access penalty.
  //return (valid && tag == TAG(addr));  // was it a hit?
}

// ------------------------------------------------------------------------
// Below is the implementation of Cache's methods.
// Caches don't do much besides pass the buck along to the appropriate block
// ------------------------------------------------------------------------
  
Cache::Cache(int lines, int blockSizeInBytes, WriteBuffer& wb) :
  writeBuffer(wb)
{
    blocks = new CacheBlock[lines];  
    blocks1 = new CacheBlock[lines-512];  blocks2 = new CacheBlock[lines-1536]; 
    blocks1 = blocks; blocks2 = blocks + 1536;
    cout << "Content in blocks: " << blocks << endl; /*cout << "Content in blocks1: " << blocks1 << endl; cout << "Content in blocks2: " << blocks2 << endl;
    cout << "blocks + LINES: " << blocks + LINES << endl;
    cout << "sizeof(CacheBlock[lines]): " << sizeof(CacheBlock[lines]) << endl;
    cout << "sizeof(CacheBlock[lines-512]): " << sizeof(CacheBlock[lines-512]) << endl;
    cout << "sizeof(CacheBlock[lines-1536]): " << sizeof(CacheBlock[lines-1536]) << endl;
    for(int j=0; j<5; j++)  //print addresses of first 5 cacheblocks
    {
     cout << "adrs of block " << j << ": " << blocks + j*sizeof(CacheBlock)
    }*/
    numBlocks = lines;    
    blockSize = blockSizeInBytes;
    readHits = readMisses = readStallCycles = 0;
    writeHits = writeMisses = writeStallCycles = 0;
}


// ------------------------------------------------------------------------
// read:  find the block and pass the buck.  Update the statistics.
// ------------------------------------------------------------------------

int 
Cache::read(int addr) 
{
  int cycles = 0;
  int hit = 0;
  flag = 0; 
  hit = blocks[INDEX(addr)].read(addr, writeBuffer, cycles); cout << endl << "Read hit or miss: " << hit << endl;
  cout << "Content in [INDEX(addr)]: " << INDEX(addr) << endl; cout << "Content in blocks + [INDEX(addr)]: " << blocks + INDEX(addr) << endl;
  // calc blk number to send a blk in read reg
  /*hit = blocks[(INDEX(addr))%1536].read(addr, writeBuffer, cycles); cout << endl << "Read hit or miss: " << hit << endl;
  cout << "Content in (INDEX(addr))%1536]: " << (INDEX(addr))%1536 << endl; cout << "Content in blocks + (INDEX(addr))%1536]: " << blocks + (INDEX(addr))%1536 << endl;*/
  if (flag == 1)
  {
   swap(addr); //send address of cache block to be swapped
  }
  updateStats(hit, cycles, readHits, readMisses, readStallCycles);
  return hit;
}

// ------------------------------------------------------------------------
// write:  find the block and pass the buck.  Update the statistics.
// ------------------------------------------------------------------------

int 
Cache::write(int addr) 
{
  int cycles = 0;
  int hit = 0;
  hit = blocks[INDEX(addr)].write(addr, writeBuffer, cycles); cout << endl << "Write hit or miss: " << hit << endl;
  cout << "Content in [INDEX(addr)]: " << INDEX(addr) << endl; cout << "Content in blocks + [INDEX(addr)]: " << blocks + INDEX(addr) << endl;
  // calc blk number to send a blk in read reg
  /*hit = blocks[1536+(INDEX(addr))%512].write(addr, writeBuffer, cycles); cout << endl << "Write hit or miss: " << hit << endl;
  cout << "Content in [1536+(INDEX(addr))%512]: " << 1536+(INDEX(addr))%512 << endl; cout << "Content in [blocks + 1536+(INDEX(addr))%512]: " << blocks + 1536+(INDEX(addr))%512<< endl;*/
  updateStats(hit, cycles, writeHits, writeMisses, writeStallCycles);
  return hit;
}

// ------------------------------------------------------------------------
// Swap func below
// ------------------------------------------------------------------------
int 
Cache::swap(int addr) 
{
 int tmp,j;
  j = rand() % 10 + 1; //randomly select a blk with which current blk is to be replaced
 //swap tag
 tmp = blocks[INDEX(addr)].tag;
 blocks[INDEX(addr)].tag = blocks[INDEX(addr)-j].tag;
 blocks[INDEX(addr)-j].tag = tmp;

 //swap valid 
 tmp = blocks[INDEX(addr)].valid;
 blocks[INDEX(addr)].valid = blocks[INDEX(addr)-j].valid;
 blocks[INDEX(addr)-j].valid = tmp;
}

// ------------------------------------------------------------------------
// Get num of ref to  cache blk
// ------------------------------------------------------------------------

int Cache::refcnt(int addr)
{
 int c;
 c = blocks[INDEX(addr)].getrefcount();
 return c;
}

void 
Cache::updateStats(int hit, int cycles, int& hits, int& misses, int& cycleCt) 
{
  if (hit)
    hits++;
  else
    misses++;
  cycleCt = cycleCt + cycles;
}  

void 
Cache::report(ostream& os) {
  int totalHits = readHits + writeHits;
  int totalMisses = readMisses + writeMisses;
  int totalRefs = totalHits + totalMisses;
  int cycles = readStallCycles + writeStallCycles;
  os << "# Lines = " << numBlocks << " linesize = " << blockSize 
     << " bytes"<< endl;
  os << "Reads:  hits = " << readHits << ", misses = " << readMisses << endl;
  os << "Writes: hits = " << writeHits << ", misses = "<< writeMisses<< endl;
  os << "Miss rate = "<< ((float)totalMisses/totalRefs * 100.0)<< "%" << endl; 
  os << "Stall Cycles: reads = " << readStallCycles <<
    ", writes = " << writeStallCycles << ", total = " << cycles << endl;
  os << "Total accesses = " << totalRefs << endl;
}

// ------------------------------------------------------------------------
// Main.
// The format of the data file is <type> <addr> ... <type> <addr> <eof>
// <type> is a decimal, where 0 means instruction fetch, 1 means data load
// and 2 means data store.  <addr> is a hex address that is being accessed.
// Main just reads <type> <addr> pairs from standard input, dispatching
// the accesses to the cache.
// When an <eof> is encountered, the loop is exited and the cache stats are
// reported.
// ------------------------------------------------------------------------

int main () 
{
  WriteBuffer wb(WB_DEPTH, WM_PENALTY);
  Cache cache(LINES, LINESIZE, wb);
  int i, iloads, type, addr, hit;
  iloads = 0;

  while (cin >> type >> hex >> addr) {
    cout << "Tag is: " << TAG(addr) << endl;
    if (type == 0 || type == 1) {  // a load of some sort
      if (type == 0) {
	iloads++;
      }
      hit = cache.read(addr);
    }
    else {                         // store 
      hit = cache.write(addr);
    }
  }

  // print stats

  cout << "\n** Cache Statistics **\n\n";
  cache.report(cout);
  cout << "# Instructions = " << iloads << " references/ins = " <<
    ((float)cache.references()) / iloads << endl;

   //cout << "Enter adrs to see its num of references: "; cin >> hex >> addr; 
  addr = 0x7fffe2ec;
	
  /** Get number of accesses to a cache blk */
  int c = cache.refcnt(addr);
  cout << "Ref count = " << c << endl;
 return 0;
}

/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    //assert(false);
	base_address = _base_address;
	pool_size = _size;
	free_size = _size;
	frame_pool = _frame_pool;
	page_table = page_table;
	
	allocated_regions[0].base_address = base_address;
	allocated_regions[0].size=4096;
	nregions_allocated = 1;
	
	page_table->register_pool(this);
	
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    if(_size>free_size){
		assert(false);
	}
	
	unsigned long base_address_allocated = base_address + (pool_size-free_size);
	
	unsigned long pages_to_be_allocated;
    if(!(_size%4096)){
		pages_to_be_allocated = _size/4096 + 1;
	}
	else{
		pages_to_be_allocated = _size/4096;
	}

	allocated_regions[nregions_allocated].base_address = base_address_allocated;
	allocated_regions[nregions_allocated].size =pages_to_be_allocated*4096;
	
	free_size = free_size - (pages_to_be_allocated*4096);
	
	nregions_allocated++;
	
	Console::puts("Allocated region of memory.\n");

	return (base_address_allocated);
}

void VMPool::release(unsigned long _start_address) {
    assert(false);
	
	unsigned int index_of_removed;
    unsigned long size_of_released;
    unsigned long npages_released;	
	unsigned long start_address = _start_address;
	 
	for(int i=0;i<nregions_allocated;i++)
	{
		if(allocated_regions[i].base_address == start_address){
			index_of_removed = i;
			size_of_released = allocated_regions[i].size;
			break;
		}
	}		
	
	for(int i =index_of_removed+1;i<nregions_allocated;i++){
		allocated_regions[i-1] = allocated_regions[i];
	}

	nregions_allocated--;
	free_size = free_size+size_of_released;
	
	//Call free_page
	npages_released = size_of_released/4096;
	
	for(int i=0; i<npages_released;i++){
		page_table->free_page(start_address);
		start_address += 4096;
	}
	
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    //assert(false);
	if( _address >= base_address && _address < base_address + pool_size) {
        return true;
    }
    else {
        return false;
    }
	
    Console::puts("Checked whether address is part of an allocated region.\n");
}





























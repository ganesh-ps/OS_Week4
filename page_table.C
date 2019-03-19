#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
VMPool * PageTable::VMPoolHead = NULL;

void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   //assert(false);
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
   Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
   paging_enabled = 0;
   unsigned long pg_dir_frame_no= (kernel_mem_pool->get_frames(1));
   page_directory = (unsigned long *) (pg_dir_frame_no*PAGE_SIZE);
   unsigned long pg_table_frame_no= (process_mem_pool->get_frames(1));
   unsigned long *page_table = (unsigned long *) (pg_table_frame_no*PAGE_SIZE); // the page table comes right after the page directory

  // fill the first entry of the page directory
  page_directory[1023]= (unsigned long)page_directory;
  page_directory[1023]= (unsigned long)page_directory | 0x00000003;
  
  page_directory[0] = (unsigned long)page_table; // attribute set to: supervisor level, read/write, present(011 in binary)
  page_directory[0] = (unsigned long)page_directory[0] | 3;

   unsigned long address=0; // holds the physical address of where a page is
   for(int i=1; i<ENTRIES_PER_PAGE-1; i++)
   {
       page_directory[i] = 0x00000000 | 0x00000002 ;
   }
   // map the first 4MB of memory
   for(int i=0; i<ENTRIES_PER_PAGE; i++)
   {
     page_table[i] = address | 0x00000003; // attribute set to: supervisor level, read/write, present(011 in binary)
     address = address + 4096; // 4096 = 4kb
   }
   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   //assert(false);
   current_page_table=this;
   // write_cr3, read_cr3, write_cr0, and read_cr0 all come from the assembly functions`
   write_cr3((unsigned long)page_directory); // put that page directory address into CR3
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   //assert(false);
   paging_enabled = 1;
   write_cr0(read_cr0() | 0x80000000); // set the paging bit in CR0 to 1
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  //assert(false);
  unsigned int err_code = _r->err_code;
  if((err_code & 0x00000001)!=0x00000000){
    assert(false);
  }
  unsigned long * page_table;
  unsigned long err_address = read_cr2(); 
  unsigned long pg_dir_index = (err_address & 0xFFC00000)>>22;
  unsigned long pg_table_index = (err_address & 0x003FF000)>>12;

  VMPool *p = PageTable::VMPoolHead;
  int flag =1;  

  while(p=NULL){
		if(p->is_legitimate(err_address)==true){
			flag = 1;break;
		}
		else{
			flag = 0;
		}
		p=p->next_vmpool;
  }
	
	if(flag!=1){
		Console::puts("is_legit failed in pagetable.C\n");
		assert(false);
	}


  if((current_page_table->page_directory[pg_dir_index]&0x00000001)!=0x00000001){//pg_dir doesnt exists
    page_table = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE);
    /*Console::putui((unsigned long)page_table);
    Console::putui(pg_dir_index);
    assert(false);*/
    unsigned long * recursive_pg_dir = (unsigned long *) (0xFFFFF000);
    recursive_pg_dir[pg_dir_index] = (unsigned long)page_table; // attribute set to: supervisor level, read/write, present(011 in binary)
    recursive_pg_dir[pg_dir_index] = (unsigned long)recursive_pg_dir[pg_dir_index] | 0x00000003; 
  }
  else{
    page_table = (unsigned long *) ((current_page_table->page_directory[pg_dir_index])&0xFFFFF000);
    unsigned long * recursive_pg_table = (unsigned long *) ((0xFFC00000)|(pg_dir_index<<12)); 
    recursive_pg_table[pg_table_index] = (( process_mem_pool->get_frames(1) )*PAGE_SIZE) | 0x00000003;
  }
  
  Console::puts("handled page fault\n");

}

void PageTable::register_pool(VMPool * _vm_pool)
{
    //assert(false);
    if(PageTable::VMPoolHead==NULL){
	PageTable::VMPoolHead = _vm_pool;
    }
    else {//if not first add to end of list
        VMPool *p = PageTable::VMPoolHead;
        while(NULL!=p->next_vmpool) {
            p=p->next_vmpool;
        }
        p->next_vmpool=_vm_pool;
    }	
    Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) {
    //assert(false);
	unsigned long pg_dir_index = (_page_no & 0xFFC00000)>>22;
	unsigned long pg_table_index = (_page_no & 0x003FF000)>>12;
	
	unsigned long * rec_pg_table = (unsigned long *) ((0xFFC00000)|(pg_dir_index<<12));
	
	//make entry invalid 
	rec_pg_table[pg_table_index] = rec_pg_table[pg_table_index]|0x00000002;
	//get frame number by reading first 20 bits and release frame
	process_mem_pool->release_frames(rec_pg_table[pg_table_index]&0xFFFFF000);
	
	//flush TLB
	unsigned long val_CR3 = read_cr3();
	write_cr3(val_CR3);
    Console::puts("freed page\n");
}


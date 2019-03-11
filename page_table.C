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
   //assert(false);
   paging_enabled = 0;
   unsigned long pg_dir_frame_no= (kernel_mem_pool->get_frames(1));
   page_directory = (unsigned long *) (pg_dir_frame_no*PAGE_SIZE);
   unsigned long pg_table_frame_no= (kernel_mem_pool->get_frames(1));
   unsigned long *page_table = (unsigned long *) (pg_table_frame_no*PAGE_SIZE); // the page table comes right after the page directory
   unsigned int i;
  // assert(pg_dir_frame_no);
   //assert(pg_table_frame_no);
  // fill the first entry of the page directory
  page_directory[0] = (unsigned long)page_table; // attribute set to: supervisor level, read/write, present(011 in binary)
  page_directory[0] = (unsigned long)page_directory[0] | 3;
  for(i=1; i<1024; i++)
  {
    page_directory[i] = 0x00000000 | 0x00000002; // attribute set to: supervisor level, read/write, not present(010 in binary)
  }

   unsigned long address=0; // holds the physical address of where a page is

   // map the first 4MB of memory
   for(i=0; i<1024; i++)
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
  unsigned long pg_dir_address = (err_address & 0xFFC00000)>>22;
  unsigned long pg_table_address = (err_address & 0x003FF000)>>12;
  
  if((current_page_table->page_directory[pg_dir_address]&0x00000001)!=0x00000001){//pg_dir doesnt exists
    page_table = (unsigned long *) (kernel_mem_pool->get_frames(1)*PAGE_SIZE);
    current_page_table->page_directory[pg_dir_address] = (unsigned long)page_table; // attribute set to: supervisor level, read/write, present(011 in binary)
    current_page_table->page_directory[pg_dir_address] = (unsigned long)current_page_table->page_directory[pg_dir_address] | 0x00000003; 
  }
  else{
    page_table = (unsigned long *) ((current_page_table->page_directory[pg_dir_address])&0xFFFFF000);
    page_table[pg_table_address] = (( process_mem_pool->get_frames(1) )*PAGE_SIZE) | 0x00000003;
  }
  Console::puts("handled page fault\n");

}


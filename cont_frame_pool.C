/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/
/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/
ContFramePool* ContFramePool::head_pointer=NULL;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    nFreeFrames    = _n_frames;   //till allocation freeframes is number of frames
    base_frame_no  = _base_frame_no; // Where does the frame pool start
    nframes       = _n_frames; // Size of the frame pool
    info_frame_no = _info_frame_no;// Where do we store the management information?
    ninfo_frames  = _n_info_frames; // Number of information frames
    
    if(ninfo_frames==0) ninfo_frames=1; //implies given info frame number corresponds to the 1 info frame 
  
    assert(nframes<=(FRAME_SIZE*8*ninfo_frames/2));//In the info frame every 2 bits represent 1 frame. So all nframes must fit in  the given size for bitmap 
	
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
	
    // Everything ok. Proceed to mark all states to free in the bitmap
    for(int i=0; (i*4) < nframes; i++) {
        bitmap[i] = 0xFF;//4 frames per char so iterate over evry 4 frames
    }
    
    // Mark the ninfo frames as being used if it is being used
    if(_info_frame_no == 0) {
		if(ninfo_frames==0){
			bitmap[0] = 0x3F;
			nFreeFrames--;
		}
		else{
			for(int i=0; i<ninfo_frames; i++)
				bitmap[i/4] &= rotate_right(0x3F,i);
			nFreeFrames-=ninfo_frames;
		}
    }
	
    if(NULL==ContFramePool::head_pointer){
        ContFramePool::head_pointer=this;//add first object/pool to the list
    }
    else {//if not first add to end of list
        ContFramePool *p=ContFramePool::head_pointer;
        while(NULL!=p->next_pool) {
            p=p->next_pool;
        }
        p->next_pool=this;
    }
    
    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
	unsigned int required_no_of_frames =_n_frames;
	unsigned long contiguous_available_frames=0, new_head=0;
	int i,map_index,right_shift_index;
	unsigned char frame_state;
	bool contiguous;

	if(required_no_of_frames>=nFreeFrames){
	    Console::puts("ERROR:Required no of frames more than available");
	    assert(false);
	}   
	
	//Console::puts("get_frames nframes: ");Console::putui(_n_frames);Console::puts("\n");

	//Iterate over entire bitmap to find n contiguously available frames

	for(i = 0; i < nframes; i++){
	    map_index   = i/4; //index for bitmap
	    right_shift_index = (3-(i%4))*2; // i%4 gives frame number within the byte; 2 is shift value(number of bits per frame)
  
	    /*Console::puts("get_frame map_index: ");Console::putui(map_index);Console::puts("\t");
	    Console::puts("get_frame state_index: ");Console::putui(right_shift_index);Console::puts("\t");
	    Console::puts("get_frame bitmap[i/4]: ");Console::putui(bitmap[map_index]);Console::puts("\n");*/

	    frame_state = (bitmap[map_index]>>right_shift_index) & 0x03; // shift to last two bits and read

	    if(frame_state == 0x03) //11 implies free/available
	    	contiguous_available_frames++;   // count available_frames till you hit the required value
	    else
		contiguous_available_frames=0;  // restart count for every break in contiguous_available_frames

	    if(contiguous_available_frames == required_no_of_frames){
		new_head = i-required_no_of_frames + 1 + base_frame_no; // tail - sequence length + 1
		contiguous = true;
		//Console::puts("new_head: ");Console::putui(new_head);Console::puts("\n");
		break;
	    }
	}
	
	assert(contiguous== true);//exit if no contiguously available frames exist
	
	allocate_frames(new_head,required_no_of_frames);//calling to allocate the contiguous memory

	return (new_head);
}

//Similar to function above the return of the iterator(i.e at break;) in get frames is new_head here it is true or false
bool ContFramePool::isContigious(unsigned long _base_frame_no,unsigned long _n_frames)
{
	int i,map_index,right_shift_index;
	unsigned long available_frames=0;
	unsigned char frame_state;

	for(i=_base_frame_no;i<_base_frame_no+_n_frames; i++){
		//Console::puts("Entered mark_inaccessible: ")//;Console::putui(_base_frame_no);Console::puts("\t");
		map_index=i/4;
		right_shift_index = (3-(i%4))*2; // i%4 gives freame number within the byte; 2 is shift value(number of bits per frame) 

		frame_state = (bitmap[map_index]>>right_shift_index) & 0x03; // shift to last two bits and read

		//Console::puts("Entered isContigious: ");Console::putui(frame_state);Console::puts("\n");
		if(frame_state == 0x03) //11 implies free/available
			available_frames++;
		else
			available_frames=0;
		if(available_frames==_n_frames){
			return true;
		}
	}
    
	return false;
}
//IMPLEMENTED AS DESCRIBED IN THE DETAILED IMPLEMENTATION SECTION ABOVE
//Just checks contiguity and sends the base frame number and n frames to allocate frames
void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
	Console::puts("Entered mark_inaccessible: \n");//`Console::putui(_base_frame_no);Console::puts("\t");Console::putui(_n_frames);Console::puts("\n\n");
	unsigned long start_frame = _base_frame_no - base_frame_no;
	if(_n_frames>0){
		assert(isContigious( start_frame, _n_frames));
		allocate_frames(_base_frame_no,_n_frames);
	}
	
}

//If its the first frame mark it as head else mark as allocated. Do this only if free
void ContFramePool::allocate_frames(unsigned long _base_frame_no,unsigned long _n_frames)
{
	unsigned long head_of_seq = _base_frame_no - base_frame_no;
	unsigned long length_of_seq = _n_frames;
	int i,map_index,right_shift_index;
	unsigned char frame_state;

	//Console::puts("head_of_seq: ");Console::putui(head_of_seq);Console::puts("\n");

	for(int i = head_of_seq; i< head_of_seq + length_of_seq ; i++){
		map_index   = i/4;
		right_shift_index = (3-(i%4))*2; // i%4 gives freame number within the byte; 2 is shift value(number of bits per frame)
  
		/*Console::puts("allocate map_index: ");Console::putui(map_index);Console::puts("\t");
		Console::puts("allocate state_index: ");Console::putui(right_shift_index);Console::puts("\t");
		Console::puts("allocate bitmap[i/4]: ");Console::putui(bitmap[map_index]);Console::puts("\n");*/

		frame_state = (bitmap[map_index]>>right_shift_index) & 0x03; // shift to last two bits and read

		assert(0x03==frame_state);//exit if not free='11'	

		if(i==head_of_seq)
		    bitmap[map_index] = bitmap[map_index] & rotate_right(0x7F,i); //mark '01'= head
		else
		    bitmap[map_index] = bitmap[map_index] & rotate_right(0x3F,i); //mark '00'= occupied
	    
		/*Console::puts("allocate rot right: ");Console::putui(rotate_right(0x3F,i));Console::puts("\n");	
		Console::puts("allocate i_alloc_frames: ");Console::putui(i);Console::puts("\t");
		Console::puts("allocate bitmap[i]: ");Console::putui(bitmap[i/4]);Console::puts("\n");*/
	}
	nFreeFrames -= _n_frames;
}

//used to rotate/circular shift masks by 2 bits aka frame bits 
unsigned char ContFramePool::rotate_right(unsigned char _mask, int _iterator)
{
	unsigned char i = (unsigned char) _iterator;
	unsigned int left_shift_by, right_shift_by;

	left_shift_by = (4-i%4)*2;
	right_shift_by = (i%4)*2;

	return ((_mask<<left_shift_by)&(0xFF))|(_mask>>right_shift_by);
}

//Identify pool by checking if within start and end frame number of the pool
void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
    ContFramePool *p=NULL;
    for(p=ContFramePool::head_pointer; p!=NULL; p=p->next_pool) {
        if( (_first_frame_no >= p->base_frame_no) && (_first_frame_no < (p->base_frame_no + p->nframes) ) ) {
            //TODO: Make the release_frame fucntion return 1 and do error checking
            (*p).release_frames_of_pool(_first_frame_no);
            return;
        }
    }
	assert(p==NULL);
}

//Pool specific. IF the first frame is not head exit.
//Else mark everything free till end of sequence (i.e till new head or free frame is seen)
void ContFramePool::release_frames_of_pool(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
	unsigned long start_frame = _first_frame_no - base_frame_no;
	int i,map_index,right_shift_index;
	unsigned char frame_state;

	//Console::puts("release_frames start_frame: ");Console::putui(start_frame);Console::puts("\n");
	
	//is it head of frame??
	//Console::putui(bitmap[map_index]);Console::puts("\n");

	//mark rest of sequence free
	for(int i= start_frame; i < nframes; i++){  
	    map_index   = i/4;
	    right_shift_index = (3-(i%4))*2; // i%4 gives freame number within the byte; 2 is shift value(number of bits per frame)
  
	    /*Console::puts("release map_index: ");Console::putui(map_index);Console::puts("\t");
	    Console::puts("release state_index: ");Console::putui(i%4);Console::puts("\t");
	    Console::puts("release bitmap[i/4]: ");Console::putui(bitmap[map_index]);Console::puts("\n");*/

	    frame_state = (bitmap[map_index]>>right_shift_index) & 0x03; // shift to last two bits and read

	    if(i==start_frame){

		assert(0x01==frame_state);//exit if not head='01'	
	    }

	    if((frame_state==0x03)|(frame_state==0x01)){ 
		//Console::puts("Breaking from releasing\n");
		break;
	    }
	    //assert(frame_state==0x02);
	    bitmap[map_index] = bitmap[map_index] ^ rotate_right(0x3F,i);

	    nFreeFrames++;

/*	    Console::puts("bitmap after release: ");Console::putui(bitmap[i]);Console::puts("\t");
	    Console::puts("Free frames after release: ");Console::putui(nFreeFrames);Console::puts("\n");*/
	}
}

//Explained in header file and document. Please refer
unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    unsigned long nbits_per_frame= (FRAME_SIZE*8);
    unsigned long nbits_for_n_frames= (_n_frames*2);
    unsigned long ninfo_frames_needed = (nbits_for_n_frames/nbits_per_frame) + ( ( (nbits_for_n_frames)%nbits_per_frame) > 0 ? 1 : 0 );
    //Console::puts("ninfoframes proc pool: ");Console::putui(ninfo_frames_needed);Console::puts("\n");
    return ninfo_frames_needed;
}


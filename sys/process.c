//#include <sys/elf64.h>

#include <sys/process.h>
#include <sys/tarfs.h>
#include <sys/defs.h>
#include <sys/string.h>
#include <sys/task.h>
#include <sys/pmap.h>
#include <sys/elf64.h>
#include <sys/kprintf1.h>
#include <sys/gdt.h>
#include <sys/mem.h>

pcb *t1,*t2,*t3;
pcb all_tasks[1000];
uint64_t pid=0;
vm_struct *vm= NULL;

void schedule()
{       
	pcb *next_task=NULL;
	int flag=0;
	for(int i=(curr_task->pid)+1;i<1000;i++)
	{
		if(all_tasks[i].state==RUNNING_P)
		{
			flag=1;
			next_task= &all_tasks[i];
			break;
		}
	}
	if(!flag)
	{
		flag=0;
		for(int i=0;i<=(curr_task->pid);i++)
		{
			 if(all_tasks[i].state==RUNNING_P)
                	{       
				flag=1;
                        	next_task= &all_tasks[i];
				break;
                	}
		}
	}
	
	if(flag)
	{
		pcb *temp=curr_task;
		curr_task=next_task;
                set_tss_rsp( &curr_task->kstack[510]);
		if (next_task->kstack[511] == 92736)
		{
			next_task->rsp = (uint64_t) &(next_task->kstack[496-8-15]);
			next_task->kstack[511] = 0;
		} 
        	context_switch_routine(temp, next_task);
	}
	return;
}

void tlb_flush()
{
	__asm__ volatile("movq %%cr3, %%rax; movq %%rax, %%cr3":::"%rax");
}

void initialise_tasks()
{
	for(int i=0;i<1000;i++)
	{
		all_tasks[i].pid= i;
	}
	return;
}
pcb* new_task()
{
	for(int i=0;i<1000;i++)
	{
		if(all_tasks[i].state==UNALLOCATED_P)
		{
			memset((uint64_t)&all_tasks[i], 0, sizeof(pcb));
			all_tasks[i].pid = i;
			all_tasks[i].state =ALLOCATED_P;
			return &all_tasks[i]; 
		}
	}
	kprintf_k("No processes can be accomodated\n");
	return NULL;
}

void run_queue_add(pcb *task)
{
	task->state=RUNNING_P;
}





/*
void  context_switch_routine(pcb *current_task,pcb *next_task)
{
        t3= (pcb *)page_alloc_k();
        memset((uint64_t)t3, 0, sizeof(struct pcb));


        t3->kstack[511] = (uint64_t) (&task_switcher);

        t3->rsp= (uint64_t)&(t3->kstack[511]);
        current_task->rsp= (uint64_t)&(current_task->kstack[511-15]);
        next_task->rsp= (uint64_t)&(next_task->kstack[511-15]);

        task_switcher(t1,current_task);
}
void task_switcher(pcb *current_task,pcb *next_task)
{
        schedule(current_task,next_task);
        kprintf("Done with it\n");
        return;
}
*/
void function1()
{       
        kprintf_k("HEY I AM TASK1 -- In function task-1 now \n");
        schedule(t1,t2);
        kprintf("Done with it\n");
        return;
}
void function2()
{
        
        kprintf_k("HEY I AM TASK2 -- In function task-2 now \n");
        schedule(t2,t3);
        kprintf_k("HEY I AM TASK2  - Time 2");
        schedule(t2,t3);
        kprintf_k("HEY I AM TASK2  - Time 3");
        schedule(t2,t3);
        return;
}
void function3()
{       
        kprintf_k("HEY I AM TASK3 -- Came from TASK2 now \n");
        schedule(t3,t2);
        kprintf_k("HEY I AM TASK3 -- Tim 2\n");
        schedule(t3,t2);
        kprintf_k("HEY I AM TASK3 -- Tim 3\n");
        while(1);
        schedule(t3,t2);
        return;

}

/*
void  context_switch()
{       
        t1= (pcb *)page_alloc_k();
        t2= (pcb *)page_alloc_k();
        t3= (pcb *)page_alloc_k();
                
        memset((uint64_t)t1, 0, sizeof(struct pcb));
        memset((uint64_t)t2, 0, sizeof(struct pcb));
        memset((uint64_t)t3, 0, sizeof(struct pcb));
        
        t1->kstack = (uint64_t*)page_alloc_k();
        t2->kstack = (uint64_t*)page_alloc_k();
        t3->kstack = (uint64_t*)page_alloc_k();
        
        t1->kstack[511] = (uint64_t) (&function1);
        t2->kstack[511] = (uint64_t) (&function2);
        t3->kstack[511] = (uint64_t) (&function3);

        t1->rsp= (uint64_t)&(t1->kstack[511]);
        t2->rsp= (uint64_t)&(t2->kstack[511-15]);
        t3->rsp= (uint64_t)&(t3->kstack[511-15]);

        function1();
}
*/











int oct2bin( char *str, int size)
{
    int n = 0;
    char *c = str;
    while (size > 0) {
        n *= 8;
        n += *c - '0';
        c++;
	size--;
    }
    return n;
}

void stub_func()
{
	kprintf_k("here i am hello !\n");
	while(1);
}
void load_userprogram_content(vm_struct *list, uint64_t user_cr3,uint64_t kernel_cr3)
{
	vm_struct *vm = list;
	while(vm)
	{
		if(vm->type==0)
		{
			//creating mapping
			uint64_t start= vm->address;
			uint64_t end =vm->address +vm->size;
			uint64_t paddr= vm->fp;
			
			uint64_t num_pages =(vm->size/PAGESIZE);
			uint64_t extra_page= vm->size% PAGESIZE;
			if(extra_page)
				num_pages+=1;
				
			for(int i=0;i<num_pages;i++)
			{
				uint64_t page= page_alloc_k();	
				//copy - content; 
				if(i==0&&num_pages==1)
				{
					//copying the first page.
					//we have to copy only half of the page.
							
					if(num_pages==1 && (end-(start&PAGEALIGN))!=PAGESIZE)
					{
						uint64_t lower_chunk =start- (start&PAGEALIGN);
						//copy 0 till start
						memset(page,0,lower_chunk);
						uint64_t total_content= end-start;
		
						//copy content from start to end
						//changing the cr3 to allowing copying
						 __asm__ volatile("movq %0, %%cr3"::"b"(kernel_cr3));
        					//tlb flush
						tlb_flush();
						memcpy((void *)(page+lower_chunk),(void*)(paddr),total_content);
						//switching back from kernel to user
						__asm__ volatile("movq %0, %%cr3"::"b"(user_cr3));
                                                //tlb flush
                                                __asm__ volatile("movq %%cr3, %%rax; movq %%rax, %%cr3":::"%rax");  
							
						uint64_t upper_chunk = PAGESIZE- total_content - lower_chunk;
						//copy 0 from end till the end of the page
						memset(page+end, 0,upper_chunk );
						//adding page to user pagetable
						page_table_walk_k(page-KERNBASE,start&PAGEALIGN,(pml4)(user_cr3+KERNBASE),0x7);  
					}
					//last page
					else if(i==num_pages-1)
					{
						//copy lower chunk with content;
						uint64_t page_begin = (start*PAGEALIGN)+i*PAGESIZE;
						uint64_t page_end = end;
						uint64_t lower_chunk = page_end- page_begin; 
						uint64_t copy_from = ((paddr)&PAGEALIGN)+ i*PAGESIZE; 
						__asm__ volatile("movq %0, %%cr3"::"b"(kernel_cr3));
                                                __asm__ volatile("movq %%cr3, %%rax; movq %%rax, %%cr3":::"%rax");  
						memcpy((void*)page,(void*)copy_from,lower_chunk);
						__asm__ volatile("movq %0, %%cr3"::"b"(user_cr3));
                                                //tlb flush
                                                __asm__ volatile("movq %%cr3, %%rax; movq %%rax, %%cr3":::"%rax");
						//copy 0 in rest of the page
						memset(page+lower_chunk,0,PAGESIZE-lower_chunk);	
						page_table_walk_k(page-KERNBASE, page_begin, (pml4)(user_cr3+KERNBASE),0x7);
					}
					//any page in between
					else
					{
						uint64_t page_begin = (start*PAGEALIGN)+i*PAGESIZE;
					//	uint64_t page_end = page_begin +PAGESIZE;
						uint64_t copy_from = ((paddr)&PAGEALIGN)+ i*PAGESIZE;
						__asm__ volatile("movq %0, %%cr3"::"b"(kernel_cr3));
                                                //tlb flush
						memcpy((void *)page, (void *)copy_from,PAGESIZE);
						__asm__ volatile("movq %0, %%cr3"::"b"(user_cr3));
                                                //tlb flush
                                                __asm__ volatile("movq %%cr3, %%rax; movq %%rax, %%cr3":::"%rax");
						page_table_walk_k(page-KERNBASE, page_begin, (pml4)(user_cr3+KERNBASE),0x7);
					}
				}
			}
		}
		vm=vm->next;
	}
	return;
}
/*	
void allocate_stack()
{
	uint64_t page=page_alloc_k();
	memset(page,0,PAGESIZE);
	page_table_walk_k(page-KERNBASE,STACKTOP-4096,7);
	return;
}
*/
void switch_to_user_mode()
{
        //changing address space. Also, needs to be
	__asm__ volatile("movq %0,%%rsp;" : "=b"(curr_task->rsp));
        set_tss_rsp( &curr_task->kstack[510]);
	pml4 *user_pml4 = (pml4 *)page_alloc_k();
	uint64_t kernel_cr3;
	 __asm__ volatile("movq %%cr3,%0":"=b"(kernel_cr3):);
	uint64_t user_stack = page_alloc_k();
	page_table_walk_k(user_stack-KERNBASE,STACKTOP-4096,(pml4)user_pml4,0x7);	
        curr_task->ustack= STACKTOP;
					
        //copy page tables of kernel to user pagetables.
	*(pml4 *)((uint64_t)user_pml4 + 511*8) = *((pml4 *)(kernel_cr3+KERNBASE+511*8));
	//now load the cr3 with the user_pml4
	curr_task->pml4_t =((uint64_t)(user_pml4))-KERNBASE;
	__asm__ volatile("movq %0, %%cr3"::"b"((uint64_t)user_pml4-KERNBASE));
	//tlb flush
	__asm__ volatile("movq %%cr3, %%rax; movq %%rax, %%cr3":::"%rax");
	uint64_t user_cr3= (uint64_t)user_pml4-KERNBASE;
	load_userprogram_content(curr_task->vm_head,user_cr3,kernel_cr3 );
	
	//copy page tables of kernel to user pagetables.
//	*(pml4 *)((uint64_t)user_pml4 + 511*(4096/512)) = *((pml4 *)(kernel_pml4+KERNBASE+511*(4096/512)));
	
 	//now load the cr3 with the user_pml4 
//	__asm__ volatile("mov %0, %%cr3"::"b"((uint64_t)user_pml4-KERNBASE));
//	uint64_t user_stack = page_alloc_k();
	kprintf_k("Loading the User .. \n");	
        __asm__ volatile ("cli;" 
			  "pushq $0x23;" //push the data segmnet
                	  "pushq %1;"
                	  "pushfq;"
			  "popq %%rax;"
			  "orq  $0x200,%%rax;"
                          "pushq %%rax;"
			  "pushq $0x2B;" //cs selector
                          "pushq %0;"
			  "movq $0x0, %%rsi;"
			  "movq $0x0, %%rdi;"
			  "iretq;" //final irteq - pops eip, cs, flags then goes to cs:ip
                      
			   ::  "r"(curr_task->rip),"r"(curr_task->ustack):"%rax");
				
}
     
//void print_task_structure(pcb *task)
int tarfs_lookup(char *tarfs,char *file,char **elf_hdr)
{
     	
	//kprintf_k("%s is the filename \n",file);
	//kprintf_k("here we have archive as %p \n",tarfs);
	char *p= tarfs;    
    while (!Mystrcmp(p + 257, "ustar", 5)) 
    {
        int filesize = oct2bin(p+0x7c,11);
        if (!Mystrcmp(p,file,Mystrlen(file)+1)) 
	{
	   //kprintf_k("found the file\n");
            *elf_hdr = p+ 512;
            return filesize;
	}
        p+=(((filesize+511) /512)+1)*512;
    }
    return 0;
}

void elf_read(Elf64_Ehdr *elf,vm_struct **mmap)
{	
	uint64_t phdr_offset = elf->e_phoff;
	Elf64_Phdr* phdr =(Elf64_Phdr*)((uint64_t)elf + phdr_offset);
	//vm_struct temp;
	int num_phdr = elf->e_phnum;
	vm_struct *list=NULL;
	
	for(int i=0;i<num_phdr;i++)
	{
		if(phdr->p_type==1)
		{
			vm_struct *temp =(vm_struct *)page_alloc_k();
	                temp->flags= phdr->p_flags;
			//virtual address
        	        temp->address = (phdr->p_vaddr);
                	temp->size = phdr->p_memsz;
            	        temp->next= NULL;	
			temp->fp = (uint64_t)elf+ (uint64_t)(phdr->p_offset);
			temp->filesize = phdr->p_filesz; 				
			temp->type=0;
			if(list==NULL)
                        	list= temp;
                	else
                        {
				temp->next=list;
				list=temp;
			}
		}	
		phdr++;	
	}
	vm_struct *heap= (vm_struct *)page_alloc_k();
	
	heap->address= KERNBASE -((PAGESIZE)*1000);
	heap->type= 2;
	heap->size = PAGESIZE;
	if(list)
	{
		heap->next =list;
		list=heap;
	}
	
	vm_struct *stack = (vm_struct *)page_alloc_k();//;KERNBASE - (4096)*1000;
	
	stack->address = 0xfffffe0000000000;
	stack->type =1;
	stack->size = PAGESIZE;
	 if(list)
        {
                stack->next =list;
                list=stack;
        }

	*mmap=list;
	return;
}

uint64_t elf_load(char *filename,vm_struct **list)
{
	char *elf_hdr;
	int filesize;
	filesize = tarfs_lookup(&(_binary_tarfs_start),filename,&elf_hdr );
	kprintf_k("%d is the size of the file \n",filesize);
	
	Elf64_Ehdr *elf =(Elf64_Ehdr *)elf_hdr;
	elf_read( (Elf64_Ehdr *)elf , list);	
	return elf->e_entry;

}
/*
void print_task_structurepcb *task)
{
	kprintf_k("This is to print the task structure \n ");
				
	return;
}*/
void create_task(char *filename)
{
	
	pcb *task = new_task();
	run_queue_add(task);		
	//memset((uint64_t)task, 0, sizeof(struct pcb));
	task->vm_head=(vm_struct *) page_alloc_k();
      	//memset((uint64_t)task->mm, 0, sizeof(struct mm_struct));
	task->ustack =  0xfffffe0000000000;
//      	task->prev_task=NULL; 	
	//kprintf_k("%s  is the filename in createtask \n",filename);		
	vm_struct *list=NULL;	
	task->rip = elf_load(filename,&list);
	
	task->vm_head =list;
	curr_task= task;
	
		
//	while(1);
	switch_to_user_mode();	
	return;	
}
uint64_t vm_search(uint64_t addr)
{
	vm_struct *vm =curr_task->vm_head;
	int l=0;
	int flag=0;
	while(vm!= NULL&&vm->type==0)
	{	
		uint64_t start = vm->fp;
		uint64_t end = start+ vm->size;
		
		 
		if(l==0&&addr>=start &&addr<=end)
			flag=1;
		else if(vm->next!=NULL && addr>=start &&addr<=end &&l!=0)
			flag=2;
		else if(addr>=start && addr<=end && vm->next==NULL)
			flag=3;
		
		l++;
		vm = vm->next;
		
	}
	return flag;
}

void page_fault_handler(uint64_t error_num)
{
	
	int cow = copy_on_write(error_num);
	if (!cow)
	{
        	pml4 cr3;
        	uint64_t cr2;
        	__asm__ volatile("mov %%cr3,%0":"=b"(cr3):);
        	__asm__ volatile("mov %%cr2,%0":"=b"(cr2):);
        	
	        int l=0;
        	int flag=0;
		uint64_t addr=cr2+KERNBASE;
       		while(vm!= NULL)
        	{
			if(vm->type==0)
			{
			
			//virtual address
                	uint64_t start = vm->address;
                	uint64_t end = start+ vm->size;
			if(l==0 && addr>=start && addr<=end)
                	{       
				flag=1;
				break;
			}
                	else if(vm->next!=NULL && addr>=start &&addr<=end &&l!=0)
                        {
				flag=2;
				break;
			}
                	else if(addr>=start && addr<=end && vm->next==NULL)
                        {	
				flag=3;
				break;
			}	
			l++;
                	
                	}
			vm=vm->next;
        	}
		
		
        	if(flag)
        	{
                	 uint64_t start = vm->address; //virtual_address
		       	uint64_t end = (vm->size )+ start;
			//now copy one page. We have to make sure that the page is copied properly and 
			//it depends on the location of the address(page-fault address) in the vma-list.
			// its possible that the address is present in a page -half or full.
	

			uint64_t page =page_alloc_k();
			memset(page,0,PAGESIZE);
			uint64_t pa = vm->fp; //physical address

			
		 	//case-1 only half page has to be loaded and other half has to be filled with NULL;
			if(flag==1)
			{
				uint64_t page_begin = start&PAGEALIGN;
				uint64_t page_start_offset = start-page_begin; 
				uint64_t copy_from = (pa+KERNBASE);
				//now set from page_begin to start with 0's. 
				memset(page, 0 ,page_start_offset);
				//now copy_from to page_end in page
				memcpy((void *)(start),(void *)copy_from,PAGESIZE-page_start_offset);
				
				page_table_walk_k(page-KERNBASE,(start&PAGEALIGN) ,cr3,0x07 );
				//TODO: Refresh cr3 - TLB FLUSH
			}	
		  	//case:2 - when the address is the middle page (a full page)
			else if (flag==2)
			{
				uint64_t copy_from = (pa+KERNBASE)&PAGEALIGN;
				memcpy((void*)page,(void *)copy_from,PAGESIZE);
				
				page_table_walk_k( page-KERNBASE,cr2&PAGEALIGN ,cr3+KERNBASE,0x07 );
				//TODO: Refresh cr3 - TLB FLUSH	
			}
		  	//case:3 - when the address is not the last page, which is partially 
			else if(flag ==3)
			{
				uint64_t page_offset = end - (cr2&PAGEALIGN);
				//copy from page_begin to page_offset end;
				memcpy((void*)(page),(void *)((pa+KERNBASE)&PAGEALIGN),page_offset);
				memset(page+(page_offset),0,PAGESIZE-page_offset);
				page_table_walk_k(page-KERNBASE,cr2&PAGEALIGN,cr3+KERNBASE,0x07);
				//TODO: Refresh cr3- TLB FLUSH
			
			}
		}
       
		else
			kprintf_k("This is a seg-fault \n");
	}
	while(1);
	return;

}
/*	

                 uint64_t npages = 1+(vm->size/PAGESIZE);
                 uint64_t end = (vm->size )+ start;
                 uint16_t flag=0;
                 uint64_t filesize = vm->filesize;
                 for(int i=0;i<npages;i++)
                 {
                        uint64_t page = page_alloc_k();
                        memset(page, 0, PAGESIZE);
                        uint64_t fp = vm->fp;
			//TODO: CHECK THIS ONCE
                        memcpy((void *)page, (void *)( fp  & PAGEALIGN), PAGESIZE);
                        //start += page + PAGESIZE - start;
                        page_table_walk_k( page-KERNBASE,(start  & PAGEALIGN) ,*user_pml4,0x07 );
                        if(i<npages)

                         {

                                start += (start&PAGEALIGN) + PAGESIZE - start;
                                  fp+= (fp&PAGEALIGN)+  PAGESIZE - fp;
                                  if (fp>((vm->fp) +filesize))
                                {
                                        flag=1;
                                        break;
                                }
                                //start += (start&PAGEALIGN) + PAGESIZE - start;
                        }
                }
                  if(flag==1)
                {
                        memset( start,0, (end-start-filesize));
                }
                   //start += (start&PAGEALIGN) + PAGESIZE - start;

        }
	}
	*/
	


uint64_t copy_pagetables(uint64_t pml4_va)
{
#define VA(pa) ((pa)+KERNBASE)
#define PA(va) ((va)-KERNBASE)
        pml4* pml4_p= (pml4 * )(pml4_va);
        pml4* pml4_c= (pml4 *)page_alloc_k();
        memset((uint64_t)pml4_c, 0,PAGESIZE);

        pml4_c[511] = pml4_p[511];
        for(int i=0;i<511;i++)
        {
                if((pml4_p[i])&3 )
                {
                        pml4_c[i] = PA(page_alloc_k());
                        pdp *pdp_p = (pdp *)((VA(pml4_p[i]) & PAGEALIGN));
                        pdp *pdp_c = (pdp *)(VA(pml4_c[i]));
			pml4_c[i] |=7;
                        for(int j=0;j<512;j++)
                        {
                                if((pdp_p[j])&3 )
                                {
                                        pdp_c[j] = PA(page_alloc_k());
                                        pd *pd_p = (pd *)((VA(pdp_p[j])) & PAGEALIGN);
                                        pd *pd_c = (pd *)(VA(pdp_c[j]));
					pdp_c[j] |=7;
                                        for(int k=0;k<512;k++)
                                        {
                                                if((pd_p[k])&3 )
                                                {
                                                        pd_c[k] = PA(page_alloc_k());
                                                        pt *pt_p = (pt *)((VA(pd_p[k])) & PAGEALIGN);
                                                        pt *pt_c =(pt *)( VA(pd_c[k]));
                                    			pd_c[k]|=7;
				                        for(int l=0;l<512;l++)
                                                        {
								pt_c[l] = pt_p[l]|7;
								//pt_p[l] = pt_p[l] &5;
								if(((pt_c[l])&PAGEALIGN)!=0)
									count_page(pt_p[l] + KERNBASE);
							}
                                                }
                                        }

                                }


                         }

                    }

            }

	tlb_flush();
	return PA((uint64_t)pml4_c);

}

int copy_on_write(uint64_t errorno)
{
	
        if (errorno==7)
        {
                // since errocode is 7., The exception is regarding COW
                //Now, we execute COW.
                //create a new page and copy the content of the current page to the new page allocated.
                uint64_t cr2;
                uint64_t page = page_alloc_k();
                __asm__ volatile("mov %%cr2,%0; ":"=b"(cr2):);
                //uint64_t pa= cr2-KERNBASE;
		memcpy((void *)page,(void *)(cr2&PAGEALIGN),PAGESIZE);
                page_table_walk_k(page-KERNBASE, page,(curr_task->pml4_t) ,7);
		
                return 1;
        }

        return 0;
}
 void copy_vmas(vm_struct *parent,vm_struct **child)
{
        vm_struct *temp =parent,*prev=parent;
        while(temp!=NULL)
        {

                vm_struct *vm  = (vm_struct *)page_alloc_k();
                memset((uint64_t)vm, 0,PAGESIZE);
                vm->address = temp->address;
                vm->size= temp->size;
                vm->type =temp->type;
                vm->fp = temp->fp;
                vm->filesize =temp->filesize;
                if(temp==parent)
                {
                        vm->next=NULL;
                        *child = vm;
                        prev = vm;
                }
                else
                {
                        
			prev->next=vm;
                        vm->next=NULL;
			prev = prev->next;
                }
                temp=temp->next;
        }
        return;

}

uint64_t fork()
{
	pcb *parent= curr_task;	
        pcb *child= new_task();

        memcpy(child->kstack, parent->kstack, 512*8);

        //child->rsp=parent->rsp;

        //what should be the state
        child->pml4_t = copy_pagetables(parent->pml4_t + KERNBASE);
        child->parent_pid = parent->pid;
	child->ustack = parent->ustack;
	child->rip= parent->rip;
        //child->mm = (mm_struct *)page_alloc_k();
        vm_struct *list = (vm_struct *)page_alloc_k();
        memset((uint64_t)list,0,PAGESIZE);
        copy_vmas(parent->vm_head, &list);
        child->vm_head = list;

	run_queue_add(child);	
	//just to check if its a child process.
       	child->kstack[511] = 92736;
	
	if(curr_task->pid==parent->pid)
	{
		return child->pid;
	}
	
	return 0;
}


#ifndef __TASK_H__
#define __TASK_H__
#define PAGEALIGN 0xfffffffffffff000
#define STACKTOP 0xfffffe0000000000
//extern uint64_t pid=0;

 typedef struct vm_struct{
    uint64_t flags;
    void *addr;
        
    uint64_t size;
    uint64_t address;
    
    uint64_t fp;
    uint64_t filesize;

    struct vm_struct *next;
    uint16_t type; //0 = for program header info, 1 for stack, 2 for heapsiz
}vm_struct;

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp, rip, flags, cr3;
} regs;
typedef struct mm_struct{
	
    vm_struct *mmap;
   uint64_t flags; 
 
} mm_struct;

typedef struct file
{
 short level ;
 short token ;
 short bsize ;
 char fd ;
 unsigned flags ;
 unsigned char hold ;
 unsigned char *buffer ;
 unsigned char * curp ;
 unsigned istemp; 
} file ;


typedef struct pcb{
  uint64_t rsp;
  uint64_t pml4_t;
  regs cpu_regs;
  uint64_t kstack[512];
  uint64_t pid;
  enum {UNALLOCATED_P, RUNNING_P, SLEEPING_P, ZOMBIE_P, ALLOCATED_P} state;
  int exit_status;
  uint64_t ustack; 
  uint64_t rip;
  vm_struct *vm_head;
  uint64_t parent_pid;
  /* filesystem information */
  //struct fs_struct *fs;
   /* open file information */
  file *files;
} pcb;
pcb* curr_task;
uint64_t vm_search(uint64_t addr);

void initialise_tasks();
void task_switcher(pcb *current_task,pcb *next_task);
void switch_threads(pcb *t1, pcb *t2);
void function1();
void function2();
void context_switch_routine(pcb *current_task,pcb *next_task);
void context_switch();
void schedule();
void print_task_structure(pcb *task);
uint64_t elf_load(char *filename,vm_struct **list);
void copy_vmas (vm_struct *parent, vm_struct **child);
uint64_t fork();
void load_userprogram_content(vm_struct *list, uint64_t user_cr3,uint64_t kernel_cr3);

#endif

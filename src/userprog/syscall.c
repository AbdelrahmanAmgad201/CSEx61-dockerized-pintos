#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void validate_pointer(const void *ptr) {
  if (ptr == NULL || !is_user_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir, ptr) == NULL) {
    thread_exit();
  }
}

void load_arg(int *arg, struct intr_frame *f, int arg_number) {
  for (int i = 0; i < arg_number; i++) {
    int *curr_arg = (int *)((char *)f->esp + (i + 1) * sizeof(int));
    validate_pointer(curr_arg);
    arg[i] = *curr_arg;
  }
}
struct file *get_file_by_fd(int fd) {
  struct thread *cur = thread_current();
  struct list_elem *e;

  for (e = list_begin(&cur->file_list); e != list_end(&cur->file_list); e = list_next(e)) {
    struct file_elem *fdesc = list_entry(e, struct file_elem, elem);
    if (fdesc->fd == fd) {
      return fdesc->file;
    }
  }
  return NULL;
}
unsigned tell (int fd){
  struct file *file = get_file_by_fd(fd);
  if (file == NULL) return -1;
  return file_tell(file);
}
void seek (int fd, unsigned position){
  struct file *file = get_file_by_fd(fd);
  if (file == NULL )return; 
  file_seek(file,position);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  int syscall_number = *(int *) f->esp;
  int arg[3];
  
  /* can vary from 1 - 3*/
  int arg_number = 0;

  /* For each system call load the needed args then implement the functionality
    eg:
    arg_number = 1;
    load_arg(arg, f , arg_number);
    rest of code logic
  */
  switch (syscall_number){
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXIT:
    case SYS_EXEC:
    case SYS_WAIT:
    case SYS_CREATE:

    case SYS_REMOVE:
    case SYS_OPEN:
    case SYS_FILESIZE:
    case SYS_READ:
    case SYS_WRITE:
    case SYS_SEEK:
      arg_number = 2;
      load_arg(arg, f , arg_number);
      seek(arg[0],arg[1]);
      break;
    case SYS_TELL:
      arg_number = 1;
      load_arg(arg, f , arg_number);
      int position = tell(arg[0]);
      if (position == -1) return;
      f->eax = position;
      break;
    case SYS_CLOSE:
    default:
      break;
  }
  thread_exit ();
}


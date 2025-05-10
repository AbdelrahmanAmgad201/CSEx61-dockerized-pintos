#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);
struct lock filesys_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

void validate_pointer(const void *ptr) {
  if (ptr == NULL || !is_user_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir, ptr) == NULL) {
    thread_exit();
  }
}

void load_arg(void **args, struct intr_frame *f, int arg_number) {
  for (int i = 0; i < arg_number; i++) {
    void **arg_ptr = (void **)((char *)f->esp + (i + 1) * sizeof(void *));
    validate_pointer(arg_ptr);
    validate_pointer(*arg_ptr);  // validate memory the pointer points to
    args[i] = *arg_ptr;
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
int read (int fd, void *buffer, unsigned size)
{ 
  if(fd==0){
    for(int i=0;i<size ;i++){
      char ch=input_getc();
      ((char*)buffer)[i]=ch;
      if(ch=='\0')
          return i; 
    }
  }
  struct file* file=get_file_by_fd(fd);
  if(file==NULL){
    return -1;
  }
  return file_read(fd,buffer,size);
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
int size_file(int fd){
  struct file *file = get_file_by_fd(fd);
  if (file == NULL )return -1 ; 
  return file_length(file);
}
int open(char* file_name){
  if (file_name == NULL)return -1;
  lock_acquire(&filesys_lock);
  struct file *file = filesys_open(file_name);
  lock_release(&filesys_lock);
  if (file == NULL) return -1 ;
  struct thread * cur = thread_current();
    
  struct file_elem * new_file_elem = malloc(sizeof(struct file_elem));
  
  new_file_elem->fd = cur->next_fd++;
  new_file_elem->file = file;
  list_push_back(&cur->file_list,&new_file_elem->elem);
  
  return new_file_elem->fd;
}
bool 
remove (const char *file){
    if (file == NULL)return -1;
       lock_acquire(&filesys_lock);
       bool removed = filesys_remove(file);
       lock_release(&filesys_lock);
    return removed;
}
bool
create (const char *file, unsigned initial_size){
  if (file == NULL ) return -1;
        lock_acquire(&filesys_lock);
         bool created =filesys_create(file,initial_size);
        lock_release(&filesys_lock);
  return created;
  
}
int
 write (int fd, const void *buffer, unsigned size){
    if(buffer==NULL || size ==0) return -1;
    if(fd==1){
      unsigned max_size=500;
      for(unsigned i = 0 ; i < size ;i+=max_size){
        unsigned current_writen = (size-i)<max_size?(size-i) : max_size;

        putbuf((char *)buffer + i, current_writen);
      }
      return size;
    } 
    struct file *file = get_file_by_fd(fd);
    if (file == NULL )return -1;
   
    return file_write(file,buffer,size);
    
 }
 void 
 close(int fd){
    struct file* file=get_file_by_fd(fd);
        lock_acquire(&filesys_lock);
        file_close(file);
        lock_release(&filesys_lock);
   
 }
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  int syscall_number = *(int *) f->esp;
  void *arg[3];
  
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
    ///////////////////////////
    case SYS_CREATE:
          arg_number = 2;
          load_arg(arg, f ,arg_number);
          return create((char*)arg[0],(unsigned)arg[1]);
          break;
    case SYS_REMOVE: 
          arg_number =1;
          load_arg(arg, f ,arg_number);
          f->eax = remove((char*)arg[0]);
          break;
    case SYS_OPEN:
        arg_number = 1 ;
        load_arg(arg, f ,arg_number);
        f->eax= open((char*)arg[0]);
        break;
    case SYS_FILESIZE:
          arg_number = 1 ;
          load_arg(arg, f ,arg_number);
          int size = size_file((int)arg[0]);
          if(size == -1)return;
          f->eax = size;
          break;
    case SYS_READ:
        arg_number = 3 ;
          load_arg(arg, f ,arg_number);
          int file_bytes_read=read((int)arg[0],arg[1],(unsigned)arg[2]);
          if(file_bytes_read<=0)
             return
          f->eax=file_bytes_read;
          break;
    case SYS_WRITE:
        arg_number = 3 ;
          load_arg(arg, f ,arg_number);
          int file_bytes=write((int)arg[0],arg[1],(unsigned)arg[2]);
          f->eax = file_bytes;
          break;
    case SYS_SEEK:
      arg_number = 2;
      load_arg(arg, f , arg_number);
      seek((int)arg[0],(unsigned)arg[1]);
      break;
    case SYS_TELL:
      arg_number = 1;
      load_arg(arg, f , arg_number);
      int position = tell((int)arg[0]);
      if (position == -1) return;
      f->eax = position;
      break;
    case SYS_CLOSE:
      arg_number = 1;
      load_arg(arg, f , arg_number);
      close((int)arg[0]); 
    default:
      break;
  }
  thread_exit ();
}


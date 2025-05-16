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
    exit(-1);
  }
}

void load_arg(int *args, struct intr_frame *f, int arg_number) {
  int *esp = f->esp; // already validated
  for (int i = 0; i < arg_number; i++) {
    int *ptr = (void *)(esp + i + 1);
    /* since all pointer are 4 bytes like int and arg could be a pointer or an integer */
    validate_buffer(ptr, sizeof(4));
    args[i] = *ptr;
  }
}

void validate_string(char *ptr) {
  while (true) {
    validate_pointer(ptr); // validate before reading
    if (*ptr == '\0') break;
    ptr++;
  }
}

void validate_buffer(char *buff, unsigned int size) {
  for (int i = 0; i < size; i++)
    validate_pointer(buff + i);
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
    //printf("\n%d\n",size);
    for(int i=0;i<size ;i++){
      // printf("\nkokok\n");
      char ch=input_getc();
      //printf("\n%c\n",ch);
      ((char*)buffer)[i]=ch;
      if(ch=='\0')
          return i; 
    }
    return size;
  }
  
  struct file* file=get_file_by_fd(fd);
  if(file==NULL){
    return -1;
  }
  return file_read(fd,buffer,size);
}

unsigned tell (int fd){
  struct file *file = get_file_by_fd(fd);
  // if (file == NULL) return -1;
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
 // printf("\topen\n");
  if (file_name == NULL)return -1;
  //lock_acquire(&filesys_lock);
 // printf("\t startttt opening\n");
  struct file *file = filesys_open(file_name);
//  printf("\t ennndd opening\n");
  //lock_release(&filesys_lock);
  if (file == NULL) {
   // printf("\t file not founnd\n");
    return -1 ;
  }
 // printf("\t file founnd\n");
  struct thread * cur = thread_current();
    
  struct file_elem * new_file_elem = malloc(sizeof(struct file_elem));
//  printf("\t thread strat\n");
  new_file_elem->fd = ++cur->next_fd;
  new_file_elem->file = file;
//  printf("\n\nfd = = %d\n\n ",new_file_elem->fd);
  list_push_back(&cur->file_list,&new_file_elem->elem);
//  printf("\tend open\n");
  return new_file_elem->fd;
}

bool 
remove (const char *file){
    if (file == NULL)return -1;
      //  lock_acquire(&filesys_lock);
       bool removed = filesys_remove(file);
      //  lock_release(&filesys_lock);
    return removed;
}

bool
create (const char *file, unsigned initial_size){
  if (file == NULL ) return -1;
    // lock_acquire(&filesys_lock);
   
    bool created =filesys_create(file,initial_size);
    // lock_release(&filesys_lock);
  return created;
}

int
 write (int fd, const void *buffer, unsigned size){
  // printf("\t\twriting %s\n", buffer);
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
    //lock_acquire(&filesys_lock);
    if(file==NULL){
      return;
    }
    
	  struct thread *cur = thread_current();
    struct list_elem *e;
    struct file_elem *fdesc;

     for (e = list_begin(&cur->file_list); e != list_end(&cur->file_list); e = list_next(e)) {
        fdesc = list_entry(e, struct file_elem, elem);
        if (fdesc->fd == fd) {
          file_close(fdesc->file);
          list_remove(e);
        }
      }
    free(fdesc);
    //lock_release(&filesys_lock);
   
 }

void exit(int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  struct thread *t = thread_current();
  if (t->my_child_info != NULL)
    t->my_child_info->child_exit_status = status;
  thread_exit();
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
 
  int *esp = f->esp;
  validate_buffer(esp, sizeof(int));
  
  int syscall_number = *esp;
  int arg[3];
  /* can vary from 1 - 3*/
  int arg_number = 0;

  switch (syscall_number){
    case SYS_HALT:
    
      shutdown_power_off();
      break;

    case SYS_EXIT:
      arg_number = 1;
      validate_buffer(esp, sizeof(int));
      load_arg(arg, f , arg_number);
      exit((int)arg[0]);
      break;

    case SYS_EXEC:
      load_arg(arg, f, 1);
      validate_string(arg[0]);
      f->eax = process_execute((char*)arg[0]);
      break;

    case SYS_WAIT:
      validate_buffer(esp, sizeof(int));
      load_arg(arg, f, 1);
      f->eax = process_wait((tid_t)arg[0]);
      break;
    
    case SYS_CREATE:
      arg_number = 2;
      load_arg(arg, f ,arg_number);
      validate_string(arg[0]);
      f->eax = create((char*)arg[0],(unsigned)arg[1]);
      break;

    case SYS_REMOVE: 
      arg_number = 1;
      load_arg(arg, f ,arg_number);
      validate_string(arg[0]);
      f->eax = remove((char*)arg[0]);
      break;

    case SYS_OPEN:
     // printf("\tstart open\n");
      arg_number = 1 ;
      load_arg(arg, f ,arg_number);
      validate_string(arg[0]);
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
      // printf("famous read\n");
      arg_number = 3 ;
      load_arg(arg, f ,arg_number);
      validate_buffer(arg[1], arg[2]);
      lock_acquire(&filesys_lock);
      int file_bytes_read=read((int)arg[0],arg[1],(unsigned)arg[2]);
      f->eax=file_bytes_read;
      lock_release(&filesys_lock);
      break;

    case SYS_WRITE:
      arg_number = 3;
      load_arg(arg, f ,arg_number);
      validate_buffer(arg[1], arg[2]);
      lock_acquire(&filesys_lock);
      int file_bytes=write((int)arg[0],arg[1],(unsigned)arg[2]);
      f->eax = file_bytes;
      lock_release(&filesys_lock);
      break;

    case SYS_SEEK:
      arg_number = 2;
      load_arg(arg, f , arg_number);
      seek((int)arg[0],(unsigned)arg[1]);
      break;

    case SYS_TELL:
      arg_number = 1;
      load_arg(arg, f , arg_number);
      unsigned position = tell((int)arg[0]);
      
      f->eax = position;
      break;

    case SYS_CLOSE:
      arg_number = 1;
      load_arg(arg, f , arg_number);
      close((int)arg[0]); 
      break;

    default:
      // printf("none valid call choosed\n");
      break;
  }
  // thread_exit ();
}


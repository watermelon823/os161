#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include "opt-A2.h"
#include <kern/errno.h>

#if OPT_A2
#include <mips/trapframe.h>
#include<synch.h>
#include <vfs.h>
#include <kern/fcntl.h>
#endif

/* this implementation of sys__exit does not do anything with the exit code */
/* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  //(void)exitcode;

#if OPT_A2
  int child_num = array_num(p->childrenP);
  for(int i=0; i<child_num; i++){
    struct proc *cur = (struct proc *) array_get(p->childrenP, i);
    cur->parentP = NULL;
      if(cur->zombied == true ){
         proc_destroy(cur);
         array_set(p->childrenP, i, NULL);
      }
  }
#endif

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

#if OPT_A2
  
  
  p->exitcode = exitcode;
  if(p->parentP != NULL && p->waitpid_called == true){
    lock_acquire(p->exit_lock);
    cv_signal(p->parent_wait,p->exit_lock);
    lock_release(p->exit_lock);
    thread_exit();
  }
  else if(p->parentP != NULL && p->waitpid_called == false){
    p->zombied = true;
    thread_exit();
  }
  else{
    proc_destroy(p);
    thread_exit();
  }

#endif
  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  //proc_destroy(p);

  //thread_exit();
  /* thread_exit() does not return, so we should never get here */
  //panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = curproc->pid;
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{

struct proc *p = curproc;
/*  int exitstatus;
  int result;
  */

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */
  if (options != 0) {
    return(EINVAL);
  }
#if OPT_A2
  int child_num = array_num(p->childrenP);
  int exitcode;
  int index_now = -1;
  for(int i=0; i<child_num; i++){
    struct proc *cur = (struct proc *) array_get(p->childrenP, i);
    if(cur->pid == pid){
      index_now = i;
      break;
    }
  }
  if(index_now < 0){
    return (ECHILD);
  }

  struct proc *now = (struct proc *) array_get(p->childrenP, index_now);
  now->waitpid_called = true;
  if(now->zombied == true){
    exitcode = now->exitcode;
    proc_destroy(now);
    *retval = pid;
    int exitstatus = _MKWAIT_EXIT(exitcode);
    int result = copyout((void *)&exitstatus,status,sizeof(int));
    if (result) {
      return(result);
    }
    array_remove(p->childrenP, index_now);
    return(0);
  }
  
  lock_acquire(now->exit_lock);
  if(now->parentP != NULL){
    cv_wait(now->parent_wait,now->exit_lock);
  }
  exitcode = now->exitcode;
  lock_release(now->exit_lock);
  proc_destroy(now);

  int exitstatus = _MKWAIT_EXIT(exitcode);
  int result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  array_remove(p->childrenP, index_now);
#endif
  /* for now, just pretend the exitstatus is 0 */
  /*exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }*/
  
  *retval = pid;
  return(0);
}

#if OPT_A2
int sys_fork(struct trapframe *tf_p, pid_t* retval){
  struct proc *p = curproc; 
  struct trapframe *tf_os = kmalloc(sizeof(struct trapframe));
  *tf_os = (struct trapframe) *tf_p;
  struct proc *child_p = proc_create_runprogram(p->p_name);
  struct addrspace *newAp;
  if(child_p == NULL){
    DEBUG(DB_SYSCALL,"proc_create_runprogram in sys_fork failed\n");
    return (EMPROC);
  }
  int er = as_copy(p->p_addrspace, &newAp);
  if(er==ENOMEM){
    DEBUG(DB_SYSCALL,"as_copy for kproc in sys_fork failed\n");
    return (ENOMEM);
  }
  //associate with the new process
  spinlock_acquire(&child_p->p_lock);
	child_p->p_addrspace = newAp;
	spinlock_release(&child_p->p_lock);

  child_p->parentP = p;
  lock_acquire(p->childNum_lock);
  array_add(curproc->childrenP,child_p,NULL);
  lock_release(p->childNum_lock);

  //void (*efp_ptr)(void*) = &enter_forked_process; 
  thread_fork("childThread",child_p, enter_forked_process,tf_os,0);
  *retval = child_p->pid;
  return (0);

}
#endif

#if OPT_A2
int sys_execv(char *progname, char **args){
  struct addrspace *as;
  struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
  int args_count=0;
  

  for(int i=0; args[i]!=NULL; i++){
    args_count++;
  }
  // kprintf("args_count in sys_execv: %d\n", args_count);


  //copy progname from userspace to kernel space
  size_t progname_size = (strlen(progname) + 1) * sizeof(char);
  char* progname_kernel = kmalloc(progname_size);
  KASSERT(progname_kernel != NULL);
  result = copyin((const_userptr_t)progname, (void *)progname_kernel, progname_size);
  if (result){
    kfree(progname_kernel);
    return ENOENT;
  }

  // copy each args element to kernel space
  size_t args_size = (args_count + 1) * sizeof(char *);
  char ** args_kernel = kmalloc(args_size);
  KASSERT(args_kernel != NULL);

  for(int i=0; i<args_count; i++){
    size_t cur_size = (strlen(args[i]) + 1) * sizeof(char);
    args_kernel[i] = kmalloc(cur_size);
    result = copyin((const_userptr_t) args[i], (void *) args_kernel[i], cur_size);
    // kprintf("In sys_execv $$$args_kernel[%d]->",i);
    // kprintf("%s\n", args_kernel[i]);
    if (result){
      kfree(progname_kernel);
      for(int j=0; j<=i; j++){
        kfree(args_kernel[j]);
      }
      kfree(args_kernel);
      return EFAULT;
    }
  }
  args_kernel[args_count] = NULL;
  



	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result){
      kfree(progname_kernel);
      for(int i=0; i<args_count; i++){
       kfree(args_kernel[i]);
      }
      kfree(args_kernel);
      return EISDIR;
  }

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
    kfree(progname_kernel);
    for(int i=0; i<args_count; i++){
      kfree(args_kernel[i]);
    }
    kfree(args_kernel);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	 struct addrspace *oldAs = curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
    kfree(progname_kernel);
    for(int i=0; i<args_count; i++){
      kfree(args_kernel[i]);
    }
    kfree(args_kernel);
		return ENOENT;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
    kfree(progname_kernel);
    for(int i=0; i<args_count; i++){
      kfree(args_kernel[i]);
    }
    kfree(args_kernel);
		return result;
	}


  //copy args from kernel to userstack
  vaddr_t *args_ptrs = kmalloc((args_count + 1) * sizeof(vaddr_t));
  for(int i= (args_count-1); i>=0; i--){
    int cur_size = (strlen(args_kernel[i]) + 1) * sizeof(char);
    cur_size = ROUNDUP(cur_size,8);
    stackptr = stackptr - cur_size;
    result = copyout((const void*) args_kernel[i], (userptr_t)stackptr, cur_size);
    // kprintf("In sys_execv ***argv[%d]->",i);
    // kprintf("%s\n", args_kernel[i]);
    if (result) {
      kfree(progname_kernel);
      for(int i=0; i<args_count; i++){
         kfree(args_kernel[i]);
      }
      kfree(args_kernel);
      kfree(args_ptrs);
		  return result;
	  }
    args_ptrs[i] = stackptr;
  }
  args_ptrs[args_count] = (vaddr_t)NULL;

  //copy arg pointers from kernel to userstack
  for(int i =args_count; i>=0; i--){
    size_t va_size = sizeof(vaddr_t);
    stackptr -= va_size ;
    result = copyout((const void*)&args_ptrs[i], (userptr_t)stackptr, va_size);
    if (result) {
      kfree(progname_kernel);
      for(int i=0; i<args_count; i++){
        kfree(args_kernel[i]);
      }
      kfree(args_kernel);
      kfree(args_ptrs);
		  return result;
	  }
  }

  //destroy old addresspace
  as_destroy(oldAs);
  kfree(progname_kernel);
  kfree(args_ptrs);

  for(int i=0; i<args_count; i++){
    kfree(args_kernel[i]);
  }
  kfree(args_kernel);


  vaddr_t stackptr1 = stackptr-8;
   stackptr1 = ROUNDUP(stackptr1,8);

	/* Warp to user mode. */
	enter_new_process(args_count /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
			  stackptr1, entrypoint);

	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;

  

}
#endif

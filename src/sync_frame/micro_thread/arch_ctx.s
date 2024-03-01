#
#  context  x86 or x86_64 save and restore
#
#  x86_64    x86    
# 0	%rbx    %ebx
# 1	%rsp    %esp
# 2	%rbp    %ebp
# 3	%r12    %esi
# 4	%r13    %edi
# 5	%r14    %eip
# 6	%r15
# 7	%rip



#if  defined(__amd64__) || defined(__x86_64__)

##
#  @brief save_context
##
	.text
	.align 4
	.globl save_context
	.type save_context, @function
save_context:
	pop  %rsi			
	xorl %eax,%eax	
	movq %rbx,(%rdi)
	movq %rsp,8(%rdi)
	push %rsi	
	movq %rbp,16(%rdi)
	movq %r12,24(%rdi)
	movq %r13,32(%rdi)
	movq %r14,40(%rdi)
	movq %r15,48(%rdi)
	movq %rsi,56(%rdi)	
	ret

	.size save_context,.-save_context

##
#  @brief restore_context
##
	.text
	.align 4
	.globl restore_context
	.type restore_context, @function
restore_context:
	movl %esi,%eax			
	movq (%rdi),%rbx
	movq 8(%rdi),%rsp
	movq 16(%rdi),%rbp
	movq 24(%rdi),%r12
	movq 32(%rdi),%r13
	movq 40(%rdi),%r14
	movq 48(%rdi),%r15
	jmp *56(%rdi)

	.size restore_context,.-restore_context

##
#  @brief replace_esp
##
	.text
	.align 4
	.globl replace_esp
	.type replace_esp, @function
replace_esp:
	movq %rsi,8(%rdi)
	ret

	.size replace_esp,.-replace_esp


#elif defined(__i386__)

##
#  @brief save_context
##
	.text
	.align 4
	.globl save_context
	.type save_context, @function
save_context:
	movl 4(%esp),%edx
	popl %ecx
	xorl %eax,%eax
	movl %ebx,(%edx)
	movl %esp,4(%edx)
	pushl %ecx
	movl %ebp,8(%edx)
	movl %esi,12(%edx)
	movl %edi,16(%edx)
	movl %ecx,20(%edx)
	ret

	.size save_context,.-save_context


##
#  @brief restore_context
##
	.text
	.align 4
	.globl restore_context
	.type restore_context, @function
restore_context:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	movl (%edx),%ebx
	movl 4(%edx),%esp
	movl 8(%edx),%ebp
	movl 12(%edx),%esi
	movl 16(%edx),%edi
	jmp *20(%edx)

	.size restore_context,.-restore_context

##
#  @brief replace_esp
##
    .text
    .align 4
    .globl replace_esp
    .type replace_esp, @function
replace_esp:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
    movl %eax,4(%edx)
    ret

    .size replace_esp,.-replace_esp


#else
#error "Linux cpu arch not supported"
#endif


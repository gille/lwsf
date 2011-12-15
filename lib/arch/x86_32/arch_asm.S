.globl lwsf_arch_thread_swap
	
lwsf_arch_thread_swap:
	/* somehow the save status is screwed up.. */
	push %eax

	mov 8(%esp), %eax // Load CTX save area
	mov %ebx, 0(%eax)
	mov %ecx, 4(%eax)
	mov %edx, 8(%eax)
	mov %esi, 12(%eax)
	mov %edi, 16(%eax)
	mov %esp, 20(%eax)
	mov %ebp, 24(%eax)
	
	mov 12(%esp), %eax  // Load new context pointer
	mov 0(%eax),  %ebx
	mov 4(%eax),  %ecx
	mov 8(%eax),  %edx
	mov 12(%eax), %esi
	mov 16(%eax), %edi
	mov 20(%eax), %esp
	mov 24(%eax), %ebp

	pop %eax
	ret
	
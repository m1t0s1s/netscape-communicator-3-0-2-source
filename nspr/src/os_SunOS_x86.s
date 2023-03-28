	.text
	.globl  getedi
getedi:
	movl    %edi,%eax
	ret
	.type   getedi,@function
	.size   getedi,.-getedi

	.globl  setedi
setedi:
	movl    4(%esp),%edi
	ret
	.type   setedi,@function
	.size   setedi,.-setedi

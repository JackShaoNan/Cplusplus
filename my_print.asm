section .text
global	my_print:
global  my_print_file:

my_print:
	;青色
	mov eax, 4
	mov ebx, 1
	mov ecx, cyan
	mov edx, cyanSize
	int 80h

	mov	eax,[esp+4]	
	mov	dword[char],eax

	mov	eax,4
	mov	ebx,1
	mov	ecx,char
	mov	edx,4
	int 	80h

	;回归默认颜色
	mov eax, 4
	mov ebx, 1
	mov ecx, defColor
	mov edx, defaultSize
	int 80h

	ret
	

my_print_file:
	;绿色
	mov eax, 4
	mov ebx, 1
	mov ecx, green
	mov edx, greenSize
	int 80h


	mov	eax,[esp+4]	
	mov	dword[char],eax

	mov	eax,4
	mov	ebx,1
	mov	ecx,char
	mov	edx,4
	int 	80h

	;回归默认颜色
	mov eax, 4
	mov ebx, 1
	mov ecx, defColor
	mov edx, defaultSize
	int 80h

	ret

section .data
	yellow: db 1bh, "[0;33;40m"
	yellowSize: equ $-yellow
	red: db 1bh, "[0;31;40m" 
	redSize: equ $-red
	green: db 1bh, "[0;32;40m" 
	greenSize: equ $-green
	cyan: db 1bh, "[0;36;40m" 
	cyanSize: equ $-cyan
	defColor: db 1bh, "[0;31;40m", 1bh, "[0m"
	defaultSize: equ $-defColor

section	.bss
	char	resd	1

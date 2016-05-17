
section .data
	fmt1 db "%d",0
	fmt2 db "%d",10,0
	msg1: db "输入项数："
	size1: 	equ $-msg1 
	msg2: db "the fibonacci numbers are: ",10
	size2: equ $-msg2
	space: db " ",10
	size3: equ $-space
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
	

section .bss
	num: resw 1
	i1: resw 1
	i2: resw 1
	digit1: resw 1
	digit2: resw 1
	digit3: resw 1
	digit4: resw 1
	digit5: resw 1
	judge: resw 1	


section .text
global main
	extern scanf
	extern printf

getint:
	push ebp

	mov ebp , esp

	sub esp , 2
	lea eax , [ ebp-2]
	push eax

	push fmt1
	call scanf

	mov ax, word[ebp-2]
	mov word[num]  , ax

	mov esp , ebp
	pop ebp
	ret

putint:
	push ebp
	mov ebp , esp

	sub esp , 2
	mov ax, word[num] 
	mov word[ebp-2], ax
	push fmt2
	call printf

	mov esp , ebp
	pop ebp
  	ret

main:

	mov eax,4
	mov ebx,1
	mov ecx,msg1
	mov edx,size1
	int 80h

	call getint
	mov ax, word[num] 
	mov word[i1], ax	;通过ax把输入的数字赋值给i1
	call fibonacci

	;mov word[num]  , ax
	;call putint


fibonacci:	;输入的数字在i1里
	;push ebp
	;mov ebp , esp
	
	;提示信息2
	mov eax,4
	mov ebx,1
	mov ecx,msg2
	mov edx,size2
	int 80h

	mov word[judge], 0 	;judge = 0

	mov sp, 0 	;把sp赋值为0(第一项)
	mov si, 1 	;把si赋值为1(第二项)
	mov di, 1 	;把di赋值为1
	mov word[i2], 0 ;把i2赋值为0(i2用于临时存储第三个数)
	
	for:		

		mov word[num], si
		cmp word[num], 10	;num与10比较
		jb	if		;如果num < 10,就直接打印
		else:
			xor dx,dx
			;num >= 10的情况，要按位输出
			mov ax, word[num]
			mov cx, 10
			div cx
			mov word[digit1], dx	;digit1:个位

			xor dx,dx
			mov cx, 10
			div cx
			mov word[digit2], dx	;digit2:十位

			xor dx,dx
			mov cx, 10
			div cx
			mov word[digit3], dx	;digit13:百位

			xor dx,dx
			mov cx, 10
			div cx
			mov word[digit4], dx	;digit4:千位

			xor dx,dx
			mov cx, 10
			div cx
			mov word[digit5], dx	;digit5:万位

			;设定输出颜色
			cmp word[judge], 0 	;judge = 0,green
			je printGreen
			
			mov eax, 4
			mov ebx, 1
			mov ecx, cyan
			mov edx, cyanSize
			int 80h
			mov word[judge], 0
			jmp print2

			printGreen:
			mov eax, 4
			mov ebx, 1
			mov ecx, green
			mov edx, greenSize
			int 80h
			mov word[judge], 1


			;输出
			print2:
			cmp word[digit5], 0
			je	continue1	;if(万位==0)
			else1:		;万位！=0,输出
			add word[digit5], 30h
			mov eax, 4
			mov ebx, 1
			mov ecx, digit5
			mov edx, 1
			int 80h
			jmp continue1
			

			continue1:
			cmp word[digit4], 0
			je	compare2	;if(千位==0),再判断万位
			jmp else2
			compare2:
			cmp word[digit5], 0
			je	continue2	;万位也=0,跳过
			else2:		;千位!=0 || 万位!=0,输出
			add word[digit4], 30h
			mov eax, 4
			mov ebx, 1
			mov ecx, digit4
			mov edx, 1
			int 80h

			continue2:
			cmp word[digit3], 0
			je	compare3	;if(百位==0),再判断千，万位
			jmp else3
			compare3:
			cmp word[digit4], 0
			je	compare4	;百，千都是0,比较万位
			jmp else3
			compare4:
			cmp word[digit5], 0
			je	continue3	;万位也是0,跳过
			else3:		;百位!=0 || 千位!=0 || 万位!=0,输出
			add word[digit3], 30h
			mov eax, 4
			mov ebx, 1
			mov ecx, digit3
			mov edx, 1
			int 80h

			continue3:
			cmp word[digit2], 0
			je	compare5	;十位是0,继续比较百千万
			jmp else4
			compare5:
			cmp word[digit3], 0
			je	compare6
			jmp else4
			compare6:
			cmp word[digit4], 0
			je	compare7
			jmp else4
			compare7:
			cmp word[digit5], 0
			je	continue4
			jmp else4
			else4:
			add word[digit2], 30h
			mov eax, 4
			mov ebx, 1
			mov ecx, digit2
			mov edx, 1
			int 80h

			continue4:
			add word[digit1], 30h
			mov eax, 4
			mov ebx, 1
			mov ecx, digit1
			mov edx, 1
			int 80h
			
			jmp continue
		if:
			cmp word[judge], 0 	;judge = 0,green
			je showGreen
			
			mov eax, 4
			mov ebx, 1
			mov ecx, cyan
			mov edx, cyanSize
			int 80h
			mov word[judge], 0
			jmp print1

			showGreen:
			mov eax, 4
			mov ebx, 1
			mov ecx, green
			mov edx, greenSize
			int 80h
			mov word[judge], 1

			print1:
			add word[num], 30h
			mov eax, 4
			mov ebx, 1
			mov ecx, num
			mov edx, 1
			int 80h
			

	
		continue:
		;输出空格
		mov eax, 4
		mov ebx, 1
		mov ecx, space
		mov edx, size3
		int 80h

		;回归默认颜色
		mov eax, 4
		mov ebx, 1
		mov ecx, defColor
		mov edx, defaultSize
		int 80h

		mov word[i2], si	;i2 = si
		add word[i2], sp	;i2 = i2 + sp(第三个数)
		mov sp, si		;sp = si
		mov si, word[i2]	;si = i2

		add di, 1	;di = di + 1
		cmp di, word[i1]	;di<=ax,则执行for循环
	jbe	for

	;mov esp , ebp
	;pop ebp


exii:
	mov ebx , 0
	mov eax, 1
	int 80h





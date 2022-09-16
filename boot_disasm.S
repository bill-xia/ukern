
image:     file format binary


Disassembly of section .data:

00000000 <.data>:
   0:	31 c0                	xor    %eax,%eax
   2:	8e d8                	mov    %eax,%ds
   4:	8e c0                	mov    %eax,%es
   6:	8e e0                	mov    %eax,%fs
   8:	b8 00 06 b9 00       	mov    $0xb90600,%eax
   d:	00 ba 4f 18 cd 10    	add    %bh,0x10cd184f(%edx)
  13:	bb 00 00 89 df       	mov    $0xdf890000,%ebx
  18:	bb 00 09 8e c3       	mov    $0xc38e0900,%ebx
  1d:	66 bb 00 00          	mov    $0x0,%bx
  21:	00 00                	add    %al,(%eax)
  23:	66 ba 50 41          	mov    $0x4150,%dx
  27:	4d                   	dec    %ebp
  28:	53                   	push   %ebx
  29:	66 b8 20 e8          	mov    $0xe820,%ax
  2d:	00 00                	add    %al,(%eax)
  2f:	66 b9 18 00          	mov    $0x18,%cx
  33:	00 00                	add    %al,(%eax)
  35:	cd 15                	int    $0x15
  37:	83 c7 18             	add    $0x18,%edi
  3a:	66 83 fb 00          	cmp    $0x0,%bx
  3e:	75 e9                	jne    0x29
  40:	83 c7 10             	add    $0x10,%edi
  43:	26 66 c7 05 00 00 00 	movw   $0x92e4,%es:0x0
  4a:	00 e4 92 
  4d:	0c 02                	or     $0x2,%al
  4f:	e6 92                	out    %al,$0x92
  51:	fa                   	cli    
  52:	66 0f 01 16          	lgdtw  (%esi)
  56:	f4                   	hlt    
  57:	7d 0f                	jge    0x68
  59:	20 c0                	and    %al,%al
  5b:	66 83 c8 01          	or     $0x1,%ax
  5f:	0f 22 c0             	mov    %eax,%cr0
  62:	66 ea 6a 7c 00 00    	ljmpw  $0x0,$0x7c6a
  68:	08 00                	or     %al,(%eax)
  6a:	b8 10 00 00 00       	mov    $0x10,%eax
  6f:	8e d8                	mov    %eax,%ds
  71:	8e c0                	mov    %eax,%es
  73:	8e e0                	mov    %eax,%fs
  75:	b8 00 7c 00 00       	mov    $0x7c00,%eax
  7a:	89 c4                	mov    %eax,%esp
  7c:	e8 bf 00 00 00       	call   0x140
  81:	eb fe                	jmp    0x81
  83:	f3 0f 1e fb          	endbr32 
  87:	ba f7 01 00 00       	mov    $0x1f7,%edx
  8c:	ec                   	in     (%dx),%al
  8d:	83 e0 c0             	and    $0xffffffc0,%eax
  90:	3c 40                	cmp    $0x40,%al
  92:	75 f8                	jne    0x8c
  94:	c3                   	ret    
  95:	f3 0f 1e fb          	endbr32 
  99:	57                   	push   %edi
  9a:	53                   	push   %ebx
  9b:	8b 5c 24 10          	mov    0x10(%esp),%ebx
  9f:	e8 df ff ff ff       	call   0x83
  a4:	b8 01 00 00 00       	mov    $0x1,%eax
  a9:	ba f2 01 00 00       	mov    $0x1f2,%edx
  ae:	ee                   	out    %al,(%dx)
  af:	ba f3 01 00 00       	mov    $0x1f3,%edx
  b4:	89 d8                	mov    %ebx,%eax
  b6:	ee                   	out    %al,(%dx)
  b7:	89 d8                	mov    %ebx,%eax
  b9:	c1 e8 08             	shr    $0x8,%eax
  bc:	ba f4 01 00 00       	mov    $0x1f4,%edx
  c1:	ee                   	out    %al,(%dx)
  c2:	89 d8                	mov    %ebx,%eax
  c4:	c1 e8 10             	shr    $0x10,%eax
  c7:	ba f5 01 00 00       	mov    $0x1f5,%edx
  cc:	ee                   	out    %al,(%dx)
  cd:	89 d8                	mov    %ebx,%eax
  cf:	c1 e8 18             	shr    $0x18,%eax
  d2:	83 c8 e0             	or     $0xffffffe0,%eax
  d5:	ba f6 01 00 00       	mov    $0x1f6,%edx
  da:	ee                   	out    %al,(%dx)
  db:	b8 20 00 00 00       	mov    $0x20,%eax
  e0:	ba f7 01 00 00       	mov    $0x1f7,%edx
  e5:	ee                   	out    %al,(%dx)
  e6:	e8 98 ff ff ff       	call   0x83
  eb:	8b 7c 24 0c          	mov    0xc(%esp),%edi
  ef:	b9 80 00 00 00       	mov    $0x80,%ecx
  f4:	ba f0 01 00 00       	mov    $0x1f0,%edx
  f9:	fc                   	cld    
  fa:	f2 6d                	repnz insl (%dx),%es:(%edi)
  fc:	5b                   	pop    %ebx
  fd:	5f                   	pop    %edi
  fe:	c3                   	ret    
  ff:	f3 0f 1e fb          	endbr32 
 103:	57                   	push   %edi
 104:	56                   	push   %esi
 105:	53                   	push   %ebx
 106:	8b 44 24 10          	mov    0x10(%esp),%eax
 10a:	8b 74 24 18          	mov    0x18(%esp),%esi
 10e:	c1 ee 09             	shr    $0x9,%esi
 111:	83 c6 01             	add    $0x1,%esi
 114:	89 c7                	mov    %eax,%edi
 116:	03 7c 24 14          	add    0x14(%esp),%edi
 11a:	25 00 fe ff ff       	and    $0xfffffe00,%eax
 11f:	89 c3                	mov    %eax,%ebx
 121:	39 c7                	cmp    %eax,%edi
 123:	76 17                	jbe    0x13c
 125:	56                   	push   %esi
 126:	53                   	push   %ebx
 127:	e8 69 ff ff ff       	call   0x95
 12c:	81 c3 00 02 00 00    	add    $0x200,%ebx
 132:	83 c6 01             	add    $0x1,%esi
 135:	83 c4 08             	add    $0x8,%esp
 138:	39 df                	cmp    %ebx,%edi
 13a:	77 e9                	ja     0x125
 13c:	5b                   	pop    %ebx
 13d:	5e                   	pop    %esi
 13e:	5f                   	pop    %edi
 13f:	c3                   	ret    
 140:	f3 0f 1e fb          	endbr32 
 144:	55                   	push   %ebp
 145:	57                   	push   %edi
 146:	56                   	push   %esi
 147:	53                   	push   %ebx
 148:	83 ec 0c             	sub    $0xc,%esp
 14b:	6a 00                	push   $0x0
 14d:	68 00 10 00 00       	push   $0x1000
 152:	68 00 80 00 00       	push   $0x8000
 157:	e8 a3 ff ff ff       	call   0xff
 15c:	83 c4 0c             	add    $0xc,%esp
 15f:	81 3d 00 80 00 00 7f 	cmpl   $0x464c457f,0x8000
 166:	45 4c 46 
 169:	75 68                	jne    0x1d3
 16b:	66 83 3d 38 80 00 00 	cmpw   $0x0,0x8038
 172:	00 
 173:	74 4b                	je     0x1c0
 175:	be 40 80 00 00       	mov    $0x8040,%esi
 17a:	bf 00 00 00 00       	mov    $0x0,%edi
 17f:	bd 00 80 00 00       	mov    $0x8000,%ebp
 184:	eb 0e                	jmp    0x194
 186:	83 c7 01             	add    $0x1,%edi
 189:	83 c6 3c             	add    $0x3c,%esi
 18c:	0f b7 45 38          	movzwl 0x38(%ebp),%eax
 190:	39 f8                	cmp    %edi,%eax
 192:	7e 2c                	jle    0x1c0
 194:	89 f3                	mov    %esi,%ebx
 196:	ff 76 08             	pushl  0x8(%esi)
 199:	ff 76 28             	pushl  0x28(%esi)
 19c:	ff 76 18             	pushl  0x18(%esi)
 19f:	e8 5b ff ff ff       	call   0xff
 1a4:	8b 46 20             	mov    0x20(%esi),%eax
 1a7:	83 c4 0c             	add    $0xc,%esp
 1aa:	3b 46 28             	cmp    0x28(%esi),%eax
 1ad:	73 d7                	jae    0x186
 1af:	8b 53 10             	mov    0x10(%ebx),%edx
 1b2:	c6 04 02 00          	movb   $0x0,(%edx,%eax,1)
 1b6:	83 c0 01             	add    $0x1,%eax
 1b9:	39 43 28             	cmp    %eax,0x28(%ebx)
 1bc:	77 f1                	ja     0x1af
 1be:	eb c6                	jmp    0x186
 1c0:	ff 15 18 80 00 00    	call   *0x8018
 1c6:	b8 00 00 00 00       	mov    $0x0,%eax
 1cb:	83 c4 0c             	add    $0xc,%esp
 1ce:	5b                   	pop    %ebx
 1cf:	5e                   	pop    %esi
 1d0:	5f                   	pop    %edi
 1d1:	5d                   	pop    %ebp
 1d2:	c3                   	ret    
 1d3:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 1d8:	eb f1                	jmp    0x1cb
	...
 1e2:	00 00                	add    %al,(%eax)
 1e4:	ff                   	(bad)  
 1e5:	ff 00                	incl   (%eax)
 1e7:	00 00                	add    %al,(%eax)
 1e9:	9a cf 00 ff ff 00 00 	lcall  $0x0,$0xffff00cf
 1f0:	00 92 cf 00 17 00    	add    %dl,0x1700cf(%edx)
 1f6:	dc 7d 00             	fdivrl 0x0(%ebp)
 1f9:	00 00                	add    %al,(%eax)
 1fb:	00 00                	add    %al,(%eax)
 1fd:	00 55 aa             	add    %dl,-0x56(%ebp)

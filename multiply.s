	; read input for x and y
	ld (x), (ff)
	ld (y), (ff)

	ld a, (y)
	ld b, 1

loop:
	push
		ld a, (total)
		ld b, (x)
		add
		ld (total), a
	pop

	sub
	jnz loop
end:
	ld a, (total)
	ld (ff), a
	ld (fe), a ; exit


total:
	db 0
x:
	db 0
y:
	db 0

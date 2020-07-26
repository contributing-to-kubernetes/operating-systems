; Infinite loop.                                                                
loop:                                                                           
    jmp loop                                                                    
                                                                                
; Fill with 0s everything from our current location ($-$$) up to the 510 byte.  
times 510-($-$$) db 0                                                           
                                                                                
; Magic number.                                                                 
dw 0xaa55 

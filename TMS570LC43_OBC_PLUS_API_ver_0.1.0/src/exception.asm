;
; exception.asm
;
;  Created on: 2019¦~11¤ë21¤é
;      Author: kusoyao
;
	.ref dabortInterrupt
	.ref prefetchInterrupt


    .sect ".kernelTEXT"
    .arm

;-----------------------------------------
    .def     _undef
    .asmfunc

_undef
	SUBS pc,lr,#4
	b _undef
    .endasmfunc


;-----------------------------------------
; precise abort
; R14 register save the instruction that generated the abort
;
;
;
    .def     _prefetch
    .asmfunc

_prefetch
	SUBS pc,lr,#4
	b _prefetch

    .endasmfunc

;-----------------------------------------
    .def     _dabort
    .asmfunc

_dabort
	SUBS pc,lr,#8
	b _dabort
    .endasmfunc

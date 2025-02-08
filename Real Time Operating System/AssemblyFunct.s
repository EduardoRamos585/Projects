;-----------------------------------------------------------------------------
; Hardware Target
;-----------------------------------------------------------------------------

; Target Platform: EK-TM4C123GXL
; Target uC:       TM4C123GH6PM
; System Clock:    40 MHz

; Hardware configuration:
; 16 MHz external crystal oscillator

;-----------------------------------------------------------------------------
; Device includes, defines, and assembler directives
;-----------------------------------------------------------------------------

   .def setPrivMode
   .def POPLR
   .def getMsp
   .def getPsp
   .def setAspBit
   .def setPSP
   .def InitStack
   .def RunStack
   .def popMLr
   .def pushRegs
   .def popRegs
   .def getR0
   .def startRTOS
   .def restartTask
   .def stopTask
   .def setTaskPriority
   .def yieldTask
   .def sleepTask
   .def lockMutex
   .def unlockMutex
   .def waitSemaphore
   .def postSemaphore
   .def taskMalloc
   .def shellPreempt
   .def shellSched
   .def shellReboot
   .def shellPidof
   .def shellKill
   .def shellIpcs
   .def shellMeminfo
   .def shellTaskRun
   .def shellPs
;-----------------------------------------------------------------------------
; Register values and large immediate values
;-----------------------------------------------------------------------------

.thumb
.const

lrVal:
	.word 0xFFFFFFFD

thumbBit:
	.word 0x01000000

setPrivMode:
		     MRS  R0, CONTROL
		     ORR  R0, R0,  #1
		     MSR  CONTROL, R0
             BX LR
POPLR:
			LDR LR, lrVal
			BX LR

getMsp:
		     MRS  R0, MSP
             BX LR
getPsp:
		     MRS  R0, PSP
		     ISB
             BX LR
setAspBit:
		     MRS  R0, CONTROL
		     ORR  R0, R0,  #2
		     ISB
		     MSR  CONTROL, R0
             BX LR
setPSP:
		     MSR  PSP, R0
		     ISB
             BX LR
InitStack:
			 SUB R0, R0, #4
			 LDR R2, thumbBit       ;; 	    STACK:			Load fake xpsr flags
			 STR R2, [R0], #-4      ;;  	 Xpsr		    push Xpsr Flags
			 STR R1, [R0], #-4	    ;; 	     PC				Store PID (fn)
			 MOV R3, #25            ;; 	     				Move LR value to reg
			 STR R3, [R0], #-4      ;; 		 LR				Store LR (Old LR overwritten with ExecResult)
			 STR R12, [R0]	        ;; 		 R12			Store R12
			 MOV R4, R0             ;; 		 R3				Set values for R0 - R3
			 MOV R0, #1             ;;       R2
			 MOV R1, #2		        ;;       R1
			 MOV R2, #3             ;;       R0
			 MOV R3, #4             ;;
			 STMDB R4!, {R0-R3}     ;; 						Store R0-R3
			 MOV R0, R4		        ;; 						Store addr at R0
			 MOV R4, #23	        ;; 	     R11 - R4	    Set value at R4
			 STMDB R0!, {R4-R11}    ;; 						Store R4 - R11
			 BX LR
RunStack:

			 LDMIA R0!, {R4-R11}
			 BX LR

popMLr:
			POP {LR}
			BX LR

pushRegs:
			MRS R0, PSP
			SUB R0, #4
			STR R11, [R0], #-4
			STR R10, [R0], #-4
			STR R9, [R0], #-4
			STR R8, [R0], #-4
			STR R7, [R0], #-4
			STR R6, [R0], #-4
			STR R5, [R0], #-4
			STR R4, [R0]
			BX LR

popRegs:
			MRS R0, PSP
			LDR R4, [R0], #4
			LDR R5, [R0], #4
			LDR R6, [R0], #4
			LDR R7, [R0], #4
			LDR R8, [R0], #4
			LDR R9, [R0], #4
			LDR R10, [R0], #4
			LDR R11, [R0], #4
			MSR PSP, R0
			BX LR
getR0:
			BX LR

startRTOS:
			SVC #22
			BX LR
restartTask:
			SVC #77
			BX LR
stopTask:
			SVC #76
			BX LR
setTaskPriority:
			SVC #79
			BX LR
yieldTask:
			SVC #10
			BX LR
sleepTask:
			SVC #12
			BX LR
lockMutex:
			SVC #23
			BX LR
unlockMutex:
			SVC #24
			BX LR
waitSemaphore:
			SVC #35
			BX LR
postSemaphore:
			SVC #36
			BX LR
taskMalloc:
			SVC #45
			BX LR
shellPreempt:
			SVC #57
			BX LR
shellSched:
			SVC #56
			BX LR
shellReboot:
			SVC #59
			BX LR
shellPidof:
			SVC #84
			BX LR
shellKill:
			SVC #69
			BX LR
shellIpcs:
			SVC #67
			BX LR
shellMeminfo:
			SVC #88
			BX LR
shellTaskRun:
			SVC #100
			BX LR
shellPs:
			SVC #101
			BX LR;















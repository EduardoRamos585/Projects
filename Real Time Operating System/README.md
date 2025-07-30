## Description
The goal of this project was to replicate the operations of a Real Time Operating Systems. For this project the TI Tiva Board C series was used. 
Of course, there was some limitations to this design, especially when it came to multithreading. Since the Tiva TM4C123GH6PM comes with only one
AMR Cortex-M4F Core, running tasks simultaneously was not an option. Thefore context switchin was necessary for this design.


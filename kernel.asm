; ** por compatibilidad se omiten tildes **
; ==============================================================================
; TALLER System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
; ==============================================================================

%include "print.mac"

global start

; COMPLETAR - Agreguen declaraciones extern según vayan necesitando
extern GDT_DESC

extern screen_draw_layout
extern cell_proc

extern IDT_DESC
extern idt_init

extern pic_reset
extern pic_enable

extern mmu_init_kernel_dir
extern test_mmu
extern rcr3
extern mmu_init_task_dir

extern tss_init
extern sched_init
extern task_init

; COMPLETAR - Definan correctamente estas constantes cuando las necesiten
%define CS_RING_0_SEL 1<<3
%define DS_RING_0_SEL 3<<3
%define KERNEL_PAGE_DIR 0x00025000 

BITS 16
;; Saltear seccion de datos
jmp start

;;
;; Seccion de datos.
;; -------------------------------------------------------------------------- ;;
start_rm_msg db     'Iniciando kernel en Modo Real'
start_rm_len equ    $ - start_rm_msg

start_pm_msg db     'Iniciando kernel en Modo Protegido'
start_pm_len equ    $ - start_pm_msg

;Datos para probar mmu
;datoA dw 0x1111
;datoB dw 0x2222
;datoC dw 0x3333
;datoD dw 0x4444

;;
;; Seccion de código.
;; -------------------------------------------------------------------------- ;;

;; Punto de entrada del kernel.
BITS 16
start:
    ; COMPLETAR - Deshabilitar interrupciones
    cli

    ; Cambiar modo de video a 80 X 50
    mov ax, 0003h
    int 10h ; set mode 03h
    xor bx, bx
    mov ax, 1112h
    int 10h ; load 8x8 font

    ; COMPLETAR - Imprimir mensaje de bienvenida - MODO REAL
    ; (revisar las funciones definidas en print.mac y los mensajes se encuentran en la
    ; sección de datos)
    print_text_rm start_rm_msg, start_rm_len, 0b10111101, 20, 10

    ; COMPLETAR - Habilitar A20
    ; (revisar las funciones definidas en a20.asm)
    call A20_enable

    ; COMPLETAR - Cargar la GDT
    LGDT [GDT_DESC]

    ; COMPLETAR - Setear el bit PE del registro CR0
    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    ; COMPLETAR - Saltar a modo protegido (far jump)
    jmp CS_RING_0_SEL:modo_protegido

    ; (recuerden que un far jmp se especifica como jmp CS_selector:address)
    ; Pueden usar la constante CS_RING_0_SEL definida en este archivo

BITS 32
modo_protegido:

    ; COMPLETAR - A partir de aca, todo el codigo se va a ejectutar en modo protegido
    ; Establecer selectores de segmentos DS, ES, GS, FS y SS en el segmento de datos de nivel 0
    ; Pueden usar la constante DS_RING_0_SEL definida en este archivo
    mov ax, DS_RING_0_SEL 
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    mov ss, ax

    ; COMPLETAR - Establecer el tope y la base de la pila
    mov esp, 0x25000
    mov ebp, 0x25000

    ; COMPLETAR - Imprimir mensaje de bienvenida - MODO PROTEGIDO
    print_text_pm start_pm_msg, start_pm_len, 0b10101010, 2, 2

    ; COMPLETAR - Inicializar pantalla
    call screen_draw_layout 
    
    ; Inicializamos tabla de interrupciones
    lidt [IDT_DESC]
    call idt_init

    ; Inicializamos el PIC
    call pic_reset
    call pic_enable

    ; Inicializamos MMU, hacemos identity mapping
    call mmu_init_kernel_dir
    mov cr3, eax                    ; el valor de mmu_init_kernel_dir deberia retornar en eax
    
    ; Prendemos el pin de paginacion en CR0
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    xchg bx, bx    ;$$$$$$$ BREAKPOINT $$$$$$$$

    call tss_init
    call sched_init
    call task_init

    mov ax, 0x0058 ;0000 0000 0101 1 000 --> Selector a Initial
    LTR ax       

    ; 0000 0000 0110 0000
    ; 0x0060
    ; 0 0000 0000 1100
    ;   00012            
    ;jmp 0x0012:0 salto de fe
    jmp 0x0060:0


    ; Probamos mmu con una copia en memoria
    call test_mmu

    xchg bx, bx    ;$$$$$$$ BREAKPOINT $$$$$$$$

    ; Probamos mmu_init_task_dir
    mov ebx, cr3    ;backup de cr3
    push 0x18000
    call mmu_init_task_dir
    mov cr3, eax

    print_text_pm start_pm_msg, start_pm_len, 0b10101010, 7, 7

    pop eax 
    
    xchg bx, bx    ;$$$$$$$ BREAKPOINT $$$$$$$$

    mov cr3, ebx

    xchg bx, bx    ;$$$$$$$ BREAKPOINT $$$$$$$$


    ; Habilitamos interrupciones.
    sti

    xchg bx, bx 

    ;reloj
    ;int 0x20
    ;inta ???
    ;int 0x20
    ;int 0x20
    ;int 0x20
    ;int 0x20
    ;int 0x20

    ;cambiar eax
    ;mov eax, 0x40
    int 0x58
    ;int 0x62

    xchg bx, bx 
    


    xchg bx, bx 
    ; Ciclar infinitamente 
    mov eax, 0xFFFF
    mov ebx, 0xFFFF
    mov ecx, 0xFFFF
    mov edx, 0xFFFF
    jmp $

;; -------------------------------------------------------------------------- ;;

%include "a20.asm"

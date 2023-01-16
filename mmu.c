/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================

  Definicion de funciones del manejador de memoria
*/

#include "mmu.h"
#include "i386.h"

#include "kassert.h"

static pd_entry_t* kpd = (pd_entry_t*)KERNEL_PAGE_DIR;
static pt_entry_t* kpt = (pt_entry_t*)KERNEL_PAGE_TABLE_0;

static const uint32_t identity_mapping_end = 0x003FFFFF;
static const uint32_t user_memory_pool_end = 0x02FFFFFF;

static paddr_t next_free_kernel_page = 0x100000;
static paddr_t next_free_user_page = 0x400000;

/**
 * kmemset asigna el valor c a un rango de memoria interpretado 
 * como un rango de bytes de largo n que comienza en s 
 * @param s es el puntero al comienzo del rango de memoria
 * @param c es el valor a asignar en cada byte de s[0..n-1]
 * @param n es el tamaño en bytes a asignar
 * @return devuelve el puntero al rango modificado (alias de s)
*/
static inline void* kmemset(void* s, int c, size_t n) {
  uint8_t* dst = (uint8_t*)s;
  for (size_t i = 0; i < n; i++) {
    dst[i] = c;
  }
  return dst;
}

/**
 * zero_page limpia el contenido de una página que comienza en addr
 * @param addr es la dirección del comienzo de la página a limpiar
*/
static inline void zero_page(paddr_t addr) {
  kmemset((void*)addr, 0x00, PAGE_SIZE);
}


void mmu_init(void) {}

/**
 * mmu_next_free_kernel_page devuelve la dirección de la próxima página de kernel disponible
 * @return devuelve la dirección de memoria de comienzo de la próxima página libre de kernel
 */
paddr_t mmu_next_free_kernel_page(void) {
  paddr_t nf_kernel_page = next_free_kernel_page;
  zero_page(nf_kernel_page);
  next_free_kernel_page += PAGE_SIZE;
  return nf_kernel_page;
}

/**
 * mmu_next_free_user_page devuelve la dirección de la próxima página de usuarix disponible
 * @return devuelve la dirección de memoria de comienzo de la próxima página libre de usuarix
 */
paddr_t mmu_next_free_user_page(void) {
  paddr_t nf_user_page = next_free_user_page;
  next_free_user_page += PAGE_SIZE;
  return nf_user_page;
}

/**
 * mmu_init_kernel_dir inicializa las estructuras de paginación vinculadas al kernel y
 * realiza el identity mapping
 * @return devuelve la dirección de memoria de la página donde se encuentra el directorio 
 * de páginas usado por el kernel
 */
paddr_t mmu_init_kernel_dir(void) {
  
  //init kernel page directory
  kpd[0].pt = KERNEL_PAGE_TABLE_0 >> 12;
  kpd[0].attrs = 0x3;                         //present and writtable
  
  //init kernel page table 0
  for (size_t i = 0; i < PAGE_SIZE; i += 1)
  {
    kpt[i].page = i;
    kpt[i].attrs = 0x3;
    //mmu_map_page(0x25000, i<<12, i<<12, 0x1); //Con esto queriamos ver si mmu_map_page funcionaba.
  }
  
  return KERNEL_PAGE_DIR;
}

/**
 * mmu_map_page agrega las entradas necesarias a las estructuras de paginación de modo de que
 * la dirección virtual virt se traduzca en la dirección física phy con los atributos definidos en attrs
 * @param cr3 el contenido que se ha de cargar en un registro CR3 al realizar la traducción
 * @param virt la dirección virtual que se ha de traducir en phy
 * @param phy la dirección física que debe ser accedida (dirección de destino)
 * @param attrs los atributos a asignar en la entrada de la tabla de páginas
 */

void mmu_map_page(uint32_t cr3, vaddr_t virt, paddr_t phy, uint32_t attrs) {

  pd_entry_t* dir_base = (pd_entry_t*)(CR3_TO_PAGE_DIR(cr3));
  uint32_t    dir_index = VIRT_PAGE_DIR(virt);
  pd_entry_t* dir_entry = &(dir_base[dir_index]);

  if((dir_entry->attrs & MMU_P) == 0x0){                         //no esta presente la page table: la creamos.

    dir_entry->pt = mmu_next_free_kernel_page() >> 12;
    //dir_entry->attrs = 0x3;                                      //present and writteable
    dir_entry->attrs = (attrs & 0x3F) | 0x1;  
    
  }

  pt_entry_t* page_table_base = (pt_entry_t*)(dir_entry->pt << 12);        //accedimos_dir.pt nos da 20 bits.
  uint32_t    page_table_index = VIRT_PAGE_TABLE(virt);
  pt_entry_t* page_table_entry = &(page_table_base[page_table_index]);

  page_table_entry->page = phy >> 12;
  page_table_entry->attrs = attrs | 0x1;                                        //nos aseguramos de que quede presente.

  tlbflush();
}

/**
 * mmu_unmap_page elimina la entrada vinculada a la dirección virt en la tabla de páginas correspondiente
 * @param virt la dirección virtual que se ha de desvincular
 * @return la dirección física de la página desvinculada
 */
paddr_t mmu_unmap_page(uint32_t cr3, vaddr_t virt) {

  pd_entry_t* dir_base = (pd_entry_t *)(CR3_TO_PAGE_DIR(cr3));
  uint32_t    dir_index = VIRT_PAGE_DIR(virt);
  pd_entry_t* dir_entry = &(dir_base[dir_index]);

  pt_entry_t* page_table_base = (pt_entry_t *)(dir_entry->pt << 12);        //accedimos_dir.pt nos da 20 bits.
  uint32_t    page_table_index = VIRT_PAGE_TABLE(virt);
  pt_entry_t* page_table_entry = &(page_table_base[page_table_index]);
  
  paddr_t phy_address_31_12 = page_table_entry->page << 12;      //accedimos_tabla.page nos da 20 bits.
  paddr_t phy_address_11_0 = VIRT_PAGE_OFFSET(virt);
  paddr_t phy_address = phy_address_31_12 | phy_address_11_0; //Esta direccion fisica hay que retornar

  page_table_entry->attrs = page_table_entry->attrs & !(0x1);    //marcamos la pagina como no presente

  tlbflush();

  return phy_address;
}

#define DST_VIRT_PAGE 0xA00000
#define SRC_VIRT_PAGE 0xB00000

/**
 * copy_page copia el contenido de la página física localizada en la dirección src_addr a la página física ubicada en dst_addr
 * @param dst_addr la dirección a cuya página queremos copiar el contenido
 * @param src_addr la dirección de la página cuyo contenido queremos copiar
 * 
 * Esta función mapea ambas páginas a las direcciones SRC_VIRT_PAGE y DST_VIRT_PAGE, respectivamente, realiza
 * la copia y luego desmapea las páginas. Usar la función rcr3 definida en i386.h para obtener el cr3 actual
 */
void copy_page(paddr_t dst_addr, paddr_t src_addr) {

  uint32_t cr3_actual = rcr3();

  //obs: las direcciones virtuales que nos dieron estan en el area de usuario

  mmu_map_page(cr3_actual, DST_VIRT_PAGE, dst_addr, 0x3);
  mmu_map_page(cr3_actual, SRC_VIRT_PAGE, src_addr, 0x3);

  //*((uint32_t*)DST_VIRT_PAGE) = *((uint32_t*)SRC_VIRT_PAGE);   // Esto se veia piola pero no andaba :c
  
  uint8_t* src = (uint8_t *)(SRC_VIRT_PAGE);
  uint8_t* dst = (uint8_t *)(DST_VIRT_PAGE);
  
  for (size_t i = 0; i < PAGE_SIZE; i++)
  {
    dst[i] = src[i];
  }
  
  mmu_unmap_page(cr3_actual, DST_VIRT_PAGE);
  mmu_unmap_page(cr3_actual, SRC_VIRT_PAGE);
  
}

//La agregue para testear
void test_mmu(){
  
  paddr_t pos_fisica_a = 0x00120000;
  paddr_t pos_fisica_b = 0x00180000;

  kmemset(pos_fisica_a, 5, 4);    //32 bits de 5s
  kmemset(pos_fisica_b, 11, 4);

  copy_page(pos_fisica_a, pos_fisica_b);

}

 /**
 * mmu_init_task_dir inicializa las estructuras de paginación vinculadas a una tarea cuyo código se encuentra en la dirección phy_start
 * @param phy_start es la dirección donde comienzan las dos páginas de código de la tarea asociada a esta llamada
 * @return el contenido que se ha de cargar en un registro CR3 para la tarea asociada a esta llamada
 */
paddr_t mmu_init_task_dir(paddr_t phy_start) {

  //hay que retornar el cr3 de la tarea
  paddr_t res_cr3 = 0;

  paddr_t pagina_para_dir = mmu_next_free_kernel_page();
  paddr_t pagina_para_pt_0 = mmu_next_free_kernel_page();
  paddr_t pagina_para_pila = mmu_next_free_user_page();

  //res_cr3 = pagina_para_dir & 0xFFFFF000;                   //Aca definimos que pagina_para_dir es el directorio para esta tarea
  res_cr3 = pagina_para_dir & ~(0xFFF);                   //Aca definimos que pagina_para_dir es el directorio para esta tarea

  //iniciar un dir de paginas para la tarea
  //tambien incia 1 page table para una tarea
  pd_entry_t* tarea_pd = (pd_entry_t *)pagina_para_dir;
  pt_entry_t* tarea_pt = (pt_entry_t *)pagina_para_pt_0;

  //init page directory
  tarea_pd[0].pt = pagina_para_pt_0 >> 12;
  tarea_pd[0].attrs = 0x3;                         //present and writtable
  
  //init page table 0
  for (size_t i = 0; i < PAGE_SIZE; i += 1)
  {
    tarea_pt[i].page = i;
    tarea_pt[i].attrs = 0x3;
    //mmu_map_page(0x25000, i<<12, i<<12, 0x1); //Con esto queriamos ver si mmu_map_page funcionaba
  }
  
  //las paginas de codigo van como solo lectura a partir de 0x08000000
  mmu_map_page(res_cr3, 0x08000000, phy_start, 0x5);  //code 1, (User, solo lectura, present)
  mmu_map_page(res_cr3, 0x08001000, phy_start + PAGE_SIZE, 0x5);  //code 2, (User, solo lectora, present)
  //un stack para lectura/escritura con base en 0x08003000
  mmu_map_page(res_cr3, 0x08002000, pagina_para_pila, 0x7); //pila (User, writteable, present)

  //La memoria para la pila de una tarea se obtiene del area libre de tareas
  //y la de codigo no?

  //devuelve lo que hay que cargar en CR3
  return res_cr3;
}
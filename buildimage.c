/* Author(s): <Your name(s) here>
 * Creates operating system image suitable for placement on a boot disk
*/
/* TODO: Comment on the status of your submission. Largely unimplemented */
#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512       /* floppy sector size in bytes */
#define BOOTLOADER_SIG_OFFSET 0x1fe /* offset for boot loader signature */
// more defines...

int architectureBitWidths = 32;

/* Reads in an executable file in ELF format*/
Elf32_Phdr * read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr){
	
	Elf32_Phdr* program_table_header;
	if(execfile & *execfile != NULL){
			*ehdr = (Elf32_Ehdr *) malloc(sizeof(Elf32_Ehdr));
			*execfile = fopen(filename, "rb");
			//Read e_ident
			fread(&(*ehdr->e_ident), 1, 16, *execfile);
			if(checkE_Ident(&(*ehdr->e_ident)) != -1){
				//Read each term in the ELF Header
				read_Elf_Header(ehdr, execfile);
				program_table_header = (Elf32_Phdr*)	malloc(sizeof(Elf32_Phdr));
				

			}
			else;
			//TODO: ERROR MESSAGE INVALID FORMAT



	}
 	else
		return NULL;
}

void read_Elf_Header(Elf32_Ehdr **ehdr, FILE **execfile){
		//TODO: USE DEFINES INSTEAD OF RAW VALUES
		fread(&(*ehdr->e_type), 2, 1, *execfile);
		fread(&(*ehdr->e_machine), 2, 1, *execfile);
		fread(&(*ehdr->e_version), 4, 1, *execfile);
		fread(&(*ehdr->e_entry), architectureBitWidths, 1, *execfile);
		fread(&(*ehdr->e_phoff), architectureBitWidths, 1, *execfile);
		fread(&(*ehdr->e_shoff), architectureBitWidths, 1, *execfile);
		fread(&(*ehdr->e_flags), 4, 1, *execfile);
		fread(&(*ehdr->e_ehsize), 2, 1, *execfile);
		fread(&(*ehdr->e_phentsize), 2, 1, *execfile);
		fread(&(*ehdr->e_pnum), 2, 1, *execfile);
		fread(&(*ehdr->e_shentsize), 2, 1, *execfile);
		fread(&(*ehdr->e_shnum), 2, 1, *execfile);
		fread(&(*ehdr->e_shstrndx), 2, 1, *execfile);

}


void read_Program_Header(Elf32_Phdr **program_table_header, FILE **execfile){
		//TODO: USE DEFINES INSTEAD OF RAW VALUES
		fread(&(*ehdr->e_type), 2, 1, *execfile);
		fread(&(*ehdr->e_machine), 2, 1, *execfile);
		fread(&(*ehdr->e_version), 4, 1, *execfile);
		fread(&(*ehdr->e_entry), architectureBitWidths, 1, *execfile);
		fread(&(*ehdr->e_phoff), architectureBitWidths, 1, *execfile);
		fread(&(*ehdr->e_shoff), architectureBitWidths, 1, *execfile);
		fread(&(*ehdr->e_flags), 4, 1, *execfile);
		fread(&(*ehdr->e_ehsize), 2, 1, *execfile);
		fread(&(*ehdr->e_phentsize), 2, 1, *execfile);
		fread(&(*ehdr->e_pnum), 2, 1, *execfile);
		fread(&(*ehdr->e_shentsize), 2, 1, *execfile);
		fread(&(*ehdr->e_shnum), 2, 1, *execfile);
		fread(&(*ehdr->e_shstrndx), 2, 1, *execfile);

}

int checkE_Ident(unsigned char* e_Ident){
	if(e_Ident[0] == 0x7f && e_Ident[1] == 'E' && e_Ident[2] == 'L' && e_Ident[3] == 'F' ){
		if(e_Ident[4] == ELFCLASS64)
			architectureBitWidths = 64;
		return 0;
	}else
		return -1;
}

/* Writes the bootblock to the image file */
void write_bootblock(FILE **imagefile,FILE *bootfile,Elf32_Ehdr *boot_header, Elf32_Phdr *boot_phdr){
 

}

/* Writes the kernel to the image file */
void write_kernel(FILE **imagefile,FILE *kernelfile,Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr){

 
}

/* Counts the number of sectors in the kernel */
int count_kernel_sectors(Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr){
   
    return 0;
}

/* Records the number of sectors in the kernel */
void record_kernel_sectors(FILE **imagefile,Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr, int num_sec){
    
}


/* Prints segment information for --extended option */
void extended_opt(Elf32_Phdr *bph, int k_phnum, Elf32_Phdr *kph, int num_sec){

  /* print number of disk sectors used by the image */

  
  /*bootblock segment info */
 

  /* print kernel segment info */
  

  /* print kernel size in sectors */
}
// more helper functions...

/* MAIN */
// ignore the --vm argument when implementing (project 1)
int main(int argc, char **argv){
  	FILE *kernelfile, *bootfile,*imagefile;  //file pointers for bootblock,kernel and image
  	Elf32_Ehdr *boot_header = malloc(sizeof(Elf32_Ehdr));//bootblock ELF header
  	Elf32_Ehdr *kernel_header = malloc(sizeof(Elf32_Ehdr));//kernel ELF header

  	Elf32_Phdr *boot_program_header; //bootblock ELF program header
  	Elf32_Phdr *kernel_program_header; //kernel ELF program header

  	/* build image file */
  	imagefile = fopen(IMAGE_FILE, "wb")


  	/* read executable bootblock file */  

  	/* write bootblock */  

  	/* read executable kernel file */

  	/* write kernel segments to image */

  	/* tell the bootloader how many sectors to read to load the kernel */

  	/* check for  --extended option */
  	if(!strncmp(argv[1],"--extended",11)){
		/* print info */
  	}
  
  	return 0;
} // ends main()




/* Author(s): Gabriel Chiquetto (201719050309) & Leonardo Rocha (201719050465)
 * Creates operating system image suitable for placement on a boot disk
*/
/* TODO: Comment on the status of your submission.  Approx 1/5 implemented. */
#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512				/* floppy sector size in bytes */
#define BOOTLOADER_SIG_OFFSET 0x1fe /* offset for boot loader signature */
#define WORD_SIZE 4					/* size of the word used in 32 Bit Architecture */
#define HALF_WORD_SIZE 2
#define BUFFER_SIZE 200 			/* error buffer size in bytes */

#define bootblock_arg(ARGC) ((ARGC) - 2)
#define kernel_arg(ARGC) ((ARGC) - 1)

char error_buffer[BUFFER_SIZE]; 
int architecture_bit_width = 32; 	/* may be 64 in other arcthitectures */

int handle_file_open(FILE *file_stream, const char *file_name, const char* mode);

/* Reads the contents of the elf header and store them. */
void read_elf_header(Elf32_Ehdr *ehdr_pointer, FILE *execfile)
{	
	fread(&(ehdr_pointer->e_type), HALF_WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_machine), HALF_WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_version), WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_entry), architecture_bit_width, 1, execfile);
	fread(&(ehdr_pointer->e_phoff), architecture_bit_width, 1, execfile);
	fread(&(ehdr_pointer->e_shoff), architecture_bit_width, 1, execfile);
	fread(&(ehdr_pointer->e_flags), WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_ehsize), HALF_WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_phentsize), HALF_WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_phnum), HALF_WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_shentsize), HALF_WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_shnum), HALF_WORD_SIZE, 1, execfile);
	fread(&(ehdr_pointer->e_shstrndx), HALF_WORD_SIZE, 1, execfile);
}

/* Reads the contents of a program header and store it. */
void read_program_header(Elf32_Phdr *phdr_pointer, FILE *execfile)
{
	fread(&(phdr_pointer->p_type), WORD_SIZE, 1, execfile);
	fread(&(phdr_pointer->p_offset), WORD_SIZE, 1, execfile);
	fread(&(phdr_pointer->p_vaddr), WORD_SIZE, 1, execfile);
	fread(&(phdr_pointer->p_paddr), WORD_SIZE, 1, execfile);
	fread(&(phdr_pointer->p_filesz), WORD_SIZE, 1, execfile);
	fread(&(phdr_pointer->p_memsz), WORD_SIZE, 1, execfile);
	fread(&(phdr_pointer->p_flags), WORD_SIZE, 1, execfile);
	fread(&(phdr_pointer->p_align), WORD_SIZE, 1, execfile);
}

/*
 * Function:  check_e_Ident 
 * --------------------
 * Checks the magic number
 * to verify if it's an ELF file
 * 
 *  e_Ident: first bytes in an ELF header
 *
 *  returns: zero if checked succesfully
 *           returns -1 on error (if the file isn't in proper ELF encoding)
 */
int check_e_Ident(unsigned char *e_Ident)
{
	if (e_Ident[0] == 0x7f && e_Ident[1] == 'E' && e_Ident[2] == 'L' && e_Ident[3] == 'F')
	{
		if (e_Ident[4] == ELFCLASS64)
			architecture_bit_width = 64;
		return 0;
	}
	else
		return -1;
}

/* Read all entries of the program headers and store them*/
void read_program_entries(Elf32_Phdr *phdr, uint16_t _phnum, FILE *execfile) 
{
	for (uint16_t i = 0; i < _phnum; i++)
	{
		read_program_header(&(phdr[i]), execfile);
	}
}

/*
 * Function:  read_exec_file 
 * --------------------
 * Reads in an executable file in ELF format
 * 
 *  execfile: executable file stream to be read
 *	file_name: path for the file to be open	
 *	ehdr: ELF Header reference to be stored
 * 
 *  returns: Program Header if the file was opened succesfully
 *           returns NULL if the file couldn't be open or wasn't in ELF format
 */
Elf32_Phdr *read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr)
{
	Elf32_Phdr *program_table_header;
	Elf32_Ehdr *ehdr_pointer; /* variables to enhance */
	FILE *execfile_pointer;   /* code readability     */
	uint16_t num_program_entries;

	handle_file_open(filename, "rb", *execfile);

	if (execfile != NULL && *execfile != NULL)
	{	
		ehdr = (Elf32_Ehdr **) malloc(sizeof(Elf32_Ehdr *));
		*ehdr = (Elf32_Ehdr *) malloc(sizeof(Elf32_Ehdr));

		ehdr_pointer = *ehdr;
		execfile_pointer = *execfile;

		// read e_ident
		fread(&(ehdr_pointer->e_ident), 1, EI_NIDENT, execfile_pointer);
		if (check_e_Ident(ehdr_pointer->e_ident) != -1)
		{	
			//Read each term in the ELF Header
			read_elf_header(ehdr_pointer, execfile_pointer);

			num_program_entries = (uint16_t) ehdr_pointer->e_phnum;

			program_table_header = (Elf32_Phdr *) malloc(num_program_entries * sizeof(Elf32_Phdr));
			read_program_entries(program_table_header, num_program_entries, execfile_pointer);

			return program_table_header;
		}
		else // file not in proper elf format
		{
			fprintf(stderr, "File isn't in proper ELF format: \"%s\" \n", filename);
			return NULL;
		}	
	}
	else
	{	// File open error
		snprintf(error_buffer, BUFFER_SIZE, "Could not open file \"%s\"", filename);
		perror(error_buffer);
		return NULL;
	}
}

/* Writes the bootblock to the image file */
void write_bootblock(FILE **imagefile, FILE *bootfile, Elf32_Ehdr *boot_header, Elf32_Phdr *boot_phdr)
{

}

/* Writes the kernel to the image file */
void write_kernel(FILE **imagefile, FILE *kernelfile, Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{
}

/* Counts the number of sectors in the kernel */
int count_kernel_sectors(Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{

	return 0;
}

/* Records the number of sectors in the kernel */
void record_kernel_sectors(FILE **imagefile, Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr, int num_sec)
{
}

/* Prints segment information for --extended option */
void extended_opt(Elf32_Phdr *bph, int k_phnum, Elf32_Phdr *kph, int num_sec)
{

	/* print number of disk sectors used by the image */

	/*bootblock segment info */

	/* print kernel segment info */

	/* print kernel size in sectors */
}
// more helper functions...

/* MAIN */
// ignore the --vm argument when implementing (project 1)
int main(int argc, char **argv)
{
	FILE *kernelfile, *bootfile, *imagefile;				//file pointers for bootblock,kernel and image
	Elf32_Ehdr *boot_header = malloc(sizeof(Elf32_Ehdr));   //bootblock ELF header
	Elf32_Ehdr *kernel_header = malloc(sizeof(Elf32_Ehdr)); //kernel ELF header

	Elf32_Phdr *boot_program_header;   //bootblock ELF program header
	Elf32_Phdr *kernel_program_header; //kernel ELF program header
	
	/* check if the args were used correctly */
	if (argc < 3 || argc > 4) 
	{
		fprintf(stderr, "Usage: %s %s \n", argv[0], ARGS);
		return 1;
	}
	
	/* build image file */
	handle_file_open(IMAGE_FILE, "wb", imagefile);

	/* read executable bootblock file */

	/* write bootblock */

	/* read executable kernel file */

	/* write kernel segments to image */

	/* tell the bootloader how many sectors to read to load the kernel */

	/* check for  --extended option */
	if (!strncmp(argv[1], "--extended", 11))
	{
		/* print info */
	} 

	fclose(imagefile);
	fclose(bootfile);
	fclose(kernelfile);

	return 0;
} // ends main()

/*
 * Function:  handle_file_open 
 * --------------------
 * Tries to open the given file and handle errors
 * 
 *  file_stream: pointer to assign the file_stream if the file has been opened
 *	file_name: path for the file to be open	
 *	mode: file open mode (e.g. - r, w, rb...)
 * 
 *  returns: zero if the file was opened succesfully
 *           returns -1 on error
 */
int handle_file_open(FILE *file_stream, const char* mode, const char *file_name) 
{
	file_stream = fopen(file_name, mode);
	if (file_stream == NULL) {
		snprintf(error_buffer, BUFFER_SIZE, "Could not open file \"%s\"", file_name);
		perror(error_buffer);
		return -1;
	}
	
	return 0;
}
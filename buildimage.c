/* Author(s): Gabriel Chiquetto (201719050309) & Leonardo Rocha (201719050465)
 * Creates operating system image suitable for placement on a boot disk
*/
/* TODO: Comment on the status of your submission.  Approx 2/5 implemented. */
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
#define BOOTBLOCK_IMAGE_OFFSET 0 
#define KERNEL_IMAGE_OFFSET SECTOR_SIZE

#define bootblock_arg(ARGC) ((ARGC) - 2) /* Function-like Macro to calculate bootblock filename index in argv */
#define kernel_arg(ARGC) ((ARGC) - 1) 	 /* Function-like Macro to calculate kernel filename index in argv */



char error_buffer[BUFFER_SIZE]; 
int architecture_bit_width = 4; 	/* 4 bytes = 32 Bit Architecture */

//TODO move function signatures to the upper part of the file for organization purposes
//int handle_file_open(FILE *file_stream, const char *file_name, const char* mode);

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
int handle_file_open(FILE **file_stream, const char* mode, const char *file_name) 
{	
	if(file_stream != NULL) 
	{
		*file_stream = fopen(file_name, mode);
		if (*file_stream == NULL) 
		{
			snprintf(error_buffer, BUFFER_SIZE, "Could not open file \"%s\"", file_name);
			perror(error_buffer);
			return -1;
		}
	}
			
	return 0;
}

void debug_elf(Elf32_Ehdr *ehdr_pointer, Elf32_Phdr *phdr_pointer) 
{	
	char ehdr_fields[13][20] = {"e_type", "e_machine", "e_version", "e_entry","e_phoff", "e_shoff",
			"e_flags",  "e_ehsize", "e_phentsz", "e_phnum", "e_shentsz",
			"e_shnum", "e_shstrndx"};
	char phdr_fields[9][20] = {"entry_num","p_type", "p_offset", "p_vaddr", "p_paddr","p_filesz", "p_memsz",
			"p_flags",  "p_align"};

	printf("Magic Number: ");
	for (int i = 0; i < 16; i++) 
	{
		printf("\'%02x\' ", ehdr_pointer->e_ident[i]);
	}
	printf("\n-------------------------------------------------------------------"
			"-------------------------------------------------------------------"
			"-----------------------------------\n");

	for (int i = 0; i < 13; i++)
	{
		printf("%-12s", ehdr_fields[i]);
	}
	
	printf("\n");
	printf("%-12.08x", ehdr_pointer->e_type);
	printf("%-12.08x", ehdr_pointer->e_machine);
	printf("%-12.08x", ehdr_pointer->e_version);
	printf("%-12.08x", ehdr_pointer->e_entry);
	printf("%-12.08x", ehdr_pointer->e_phoff);
	printf("%-12.08x", ehdr_pointer->e_shoff);
	printf("%-12.08x", ehdr_pointer->e_flags);
	printf("%-12.08x", ehdr_pointer->e_ehsize);
	printf("%-12.08x", ehdr_pointer->e_phentsize);
	printf("%-12.08x", ehdr_pointer->e_phnum);
	printf("%-12.08x", ehdr_pointer->e_shentsize);
	printf("%-12.08x", ehdr_pointer->e_shnum);
	printf("%-12.08x", ehdr_pointer->e_shstrndx);
	printf("\n");

	printf("\n");
	for (int i = 0; i < 9; i++)
	{
		printf("%-12s", phdr_fields[i]);
	}
	printf("\n");
	//TODO: Investigate suspicious missing 2 bytes in bless of the field p_type 
	for (uint16_t i = 0; i < ehdr_pointer->e_phnum; i++) // loop through program header sections
	{
		printf("%-12d", i);
		printf("%-12.08x", phdr_pointer[i].p_type);
		printf("%-12.08x", phdr_pointer[i].p_offset);
		printf("%-12.08x", phdr_pointer[i].p_vaddr);
		printf("%-12.08x", phdr_pointer[i].p_paddr);
		printf("%-12.08x", phdr_pointer[i].p_filesz);
		printf("%-12.08x", phdr_pointer[i].p_memsz);
		printf("%-12.08x", phdr_pointer[i].p_flags);
		printf("%-12.08x", phdr_pointer[i].p_align);
		printf("\n");
	}
	printf("\n");
	
}

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

void read_section_header(Elf32_Shdr *shdr_pointer, FILE *execfile)
{
	fread(&(shdr_pointer->sh_name), WORD_SIZE, 1, execfile);
	fread(&(shdr_pointer->sh_type), WORD_SIZE, 1, execfile);
	fread(&(shdr_pointer->sh_flags), WORD_SIZE, 1, execfile);
	fread(&(shdr_pointer->sh_offset), WORD_SIZE, 1, execfile);
	fread(&(shdr_pointer->sh_size), WORD_SIZE, 1, execfile);
	fread(&(shdr_pointer->sh_link), WORD_SIZE, 1, execfile);
	fread(&(shdr_pointer->sh_info), WORD_SIZE, 1, execfile);
	fread(&(shdr_pointer->sh_addralign), WORD_SIZE, 1, execfile);
	fread(&(shdr_pointer->sh_entsize), WORD_SIZE, 1, execfile);
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
void read_program_entries(Elf32_Phdr *phdr, uint16_t _phnum, uint32_t ph_offset, uint16_t entry_size, FILE *execfile) 
{
	for (uint16_t i = 0; i < _phnum; i++)
	{
		// Offsets to the program header entry in the execfile
		fseek(execfile, ph_offset + i*entry_size, SEEK_SET);
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
//TODO: Refactor
Elf32_Phdr *read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr)
{
	Elf32_Phdr *program_table_header;
	Elf32_Ehdr *ehdr_pointer; /* variables to enhance */
	FILE *execfile_pointer;   /* code readability     */
	uint16_t num_program_entries;

	handle_file_open(execfile, "rb", filename);
	
	if (execfile != NULL && *execfile != NULL)
	{	
		ehdr_pointer = *ehdr;
		execfile_pointer = *execfile;

		// read e_ident
		fread(ehdr_pointer->e_ident, sizeof(char), EI_NIDENT, *execfile);
		
		if (check_e_Ident(ehdr_pointer->e_ident) != -1)
		{	
			//Read each term in the ELF Header
			read_elf_header(ehdr_pointer, execfile_pointer);
			num_program_entries = (uint16_t) ehdr_pointer->e_phnum;
			program_table_header = (Elf32_Phdr *) malloc(num_program_entries * sizeof(Elf32_Phdr));
			read_program_entries(program_table_header, num_program_entries, ehdr_pointer->e_phoff, ehdr_pointer->e_phentsize ,execfile_pointer);

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

/*
 * Function:  read_entry
 * --------------------
 * Reads a section or segment of the given header
 * 
 *  execfile: executable file stream
 *	buffer: the buffer with the read content
 *	offset: offset to the entry location in the file
 *  entry_size : size of the entry that will be read          
 */
void read_entry(FILE *execfile, unsigned char **buffer, uint32_t offset, uint32_t entry_size)
{		
	*buffer = (unsigned char *) calloc(entry_size, sizeof(unsigned char)); 
		
	// Offsets the file cursor from the Header table to the given entry
	fseek(execfile, offset, SEEK_SET);
	fread(*buffer, 1, entry_size, execfile);
}

// Loop through all sections; Read each Header and section content.
void read_sections(FILE *execfile, unsigned char **sections_buffer, Elf32_Shdr** sections_headers, Elf32_Ehdr *elf_header){	
	uint16_t section_header_size = elf_header->e_shentsize;
	uint16_t num_sections = elf_header->e_shnum;
	uint32_t sections_offset =  elf_header->e_shoff;

	for (int i = 0; i < num_sections; i++)
	{	
		sections_headers[i] = (Elf32_Shdr*) malloc(sizeof(Elf32_Shdr));
		// Offsets the file cursor from the beginning to the Section Header table
		fseek(bootfile, sections_offset + i*section_header_size, SEEK_SET);		
		read_section_header(sections_headers[i], bootfile);

		read_entry(bootfile, &(sections_buffer[i]), sections_headers[i]->sh_offset, sections_headers[i]->sh_size);
	}
}

void write_sections(FILE **imagefile, unsigned char **sections_buffer, Elf32_Shdr** sections_headers, uint32_t num_sections, uint32_t image_offset)
{
	for (int i = 0; i < num_sections; i++)
	{	
		if (sections_headers[i]->sh_addr != 0) /* This member gives the address at which the section’s first byte */ 
		{									  /* should reside. If this member == 0, the section should not be written.*/	
			// Offsets imagefile cursor from the beginning to the given section address
			fseek(*imagefile, sections_headers[i]->sh_addr + image_offset, SEEK_SET);
			fwrite(sections_buffer[i], 1, sections_headers[i]->sh_size, *imagefile);
		}
	}
}

void write_program_segments(FILE **imagefile, unsigned char **program_buffer, Elf32_Phdr *program_header, uint32_t num_programs, uint32_t image_offset)
{
	for (int i = 0; i < num_programs; i++) 		
	{
		fseek(*imagefile, boot_phdr[i].p_vaddr + image_offset, SEEK_SET);
		fwrite(program_buffer[i], 1, program_header[i].p_filesz, *imagefile);
		
		padding_size = program_header[i].p_memsz - program_header[i].p_filesz;
		
		if(padding_size > 0)
		{
			padded_buffer = (unsigned char *) calloc(padding_size, sizeof(unsigned char));
			fwrite(padded_buffer, 1, padding_size, *imagefile);
			free(padded_buffer);
		}
	}
}

void read_program_segments(FILE *execfile, unsigned char **program_buffer, Elf32_Phdr *program_header, uint16_t num_programs)
{
	for (int i = 0; i < num_programs; i++)
	{	
		read_entry(bootfile, &(program_buffer[i]), boot_phdr[i].p_offset, boot_phdr[i].p_filesz);
	}
}

//TODO: refactor this function
/* Writes the bootblock to the image file */
void write_bootblock(FILE **imagefile, FILE *bootfile, Elf32_Ehdr *boot_header, Elf32_Phdr *boot_phdr)
{	
	uint16_t num_sections = elf_header->e_shnum;
	uint16_t num_programs = boot_header->e_phnum;
	uint32_t padding_size; // When the segment size in memory is bigger than it's size in file, padding = memsz - filesz
	unsigned char *padded_buffer;
	//TODO: Verify if ** is needed
	// Allocate sections for reading
	Elf32_Shdr** sections_headers = (Elf32_Shdr**) malloc(num_sections * sizeof(Elf32_Shdr*));

	// Buffer to store the content of each section
	unsigned char **sections_buffer = (unsigned char **) malloc(num_sections * sizeof(unsigned char*));
	// Buffer to store the content of each program segment
	unsigned char **program_buffer = (unsigned char **) malloc(num_programs * sizeof(unsigned char*));

	read_program_segments(bootfile, program_buffer, boot_phdr, num_programs);
	write_program_segments(imagefile, program_buffer, boot_phdr, num_programs, BOOTBLOCK_IMAGE_OFFSET);	

	read_sections(bootfile, sections_buffer, sections_headers, boot_header);	
	write_sections(imagefile, sections_buffer, sections_headers, boot_header->e_shnum, BOOTBLOCK_IMAGE_OFFSET)
}

/* Writes the kernel to the image file */
void write_kernel(FILE **imagefile, FILE *kernelfile, Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{
	uint16_t num_sections = elf_header->e_shnum;
	uint16_t num_programs = kernel_phdr->e_phnum;
	uint32_t padding_size; // When the segment size in memory is bigger than it's size in file, padding = memsz - filesz
	unsigned char *padded_buffer;
	//TODO: Verify if ** is needed
	// Allocate sections for reading
	Elf32_Shdr** sections_headers = (Elf32_Shdr**) malloc(num_sections * sizeof(Elf32_Shdr*));

	// Buffer to store the content of each section
	unsigned char **sections_buffer = (unsigned char **) malloc(num_sections * sizeof(unsigned char*));
	// Buffer to store the content of each program segment
	unsigned char **program_buffer = (unsigned char **) malloc(num_programs * sizeof(unsigned char*));

	read_program_segments(kernelfile, program_buffer, kernel_phdr, num_programs);
	write_program_segments(imagefile, program_buffer, kernel_phdr, num_programs, KERNEL_IMAGE_OFFSET);	

	read_sections(kernelfile, sections_buffer, sections_headers, kernel_header);	
	write_sections(imagefile, sections_buffer, sections_headers, kernel_header->e_shnum, KERNEL_IMAGE_OFFSET)
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
	handle_file_open(&imagefile, "wb", IMAGE_FILE);

	/* read executable bootblock file */
	boot_program_header = read_exec_file(&bootfile, argv[bootblock_arg(argc)], &boot_header);
	debug_elf(boot_header, boot_program_header);
	/* write bootblock */
	write_bootblock(&imagefile, bootfile, boot_header, boot_program_header);

	/* read executable kernel file */
	kernel_program_header = read_exec_file(&kernelfile, argv[kernel_arg(argc)], &kernel_header);
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


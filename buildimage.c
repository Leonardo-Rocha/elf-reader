/* Author(s): Gabriel Chiquetto (201719050309) & Leonardo Rocha (201719050465)
 * Creates operating system image suitable for placement on a boot disk
*/
/* TODO: Comment on the status of your submission.  100% implemented. */
#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512						/* floppy sector size in bytes */
#define BOOTLOADER_SIG_OFFSET 0x1fe 		/* offset for boot loader signature */
#define WORD_SIZE 4							/* size of the word used in 32 Bit Architecture */
#define HALF_WORD_SIZE 2
#define BUFFER_SIZE 200 					/* error buffer size in bytes */
#define BOOTBLOCK_IMAGE_OFFSET 0 
#define KERNEL_IMAGE_OFFSET 0//SECTOR_SIZE
#define BOOTLOADER_KERNEL_SECTORS_OFFSET 2
#define TRUE 1
#define FALSE 0

#define bootblock_arg(ARGC) ((ARGC) - 2) /* Function-like Macro to calculate bootblock filename index in argv */
#define kernel_arg(ARGC) ((ARGC) - 1) 	 /* Function-like Macro to calculate kernel filename index in argv */


char error_buffer[BUFFER_SIZE]; 
int architecture_bit_width = 4; 	/* 4 bytes = 32 Bit Architecture */


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

/*
 * Function:  debug_elf 
 * --------------------
 * Prints the elf and all program headers for debug purposes
 * 
 *  ehdr_pointer: elf header 
 *	phdr_pointer: program header
 */
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

/*
 * Function:  read_elf_header 
 * --------------------
 * Reads the contents of the elf header and store them. 
 * 
 *  ehdr_pointer: elf header
 *	execfile: executable file stream to be read
 */
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

/*
 * Function:  read_program_header 
 * --------------------
 * Reads the contents of a program header and store it. 
 * 
 *  phdr_pointer: program header
 *	execfile: executable file stream to be read
 */
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
 * Function:  read_section_header 
 * --------------------
 * Reads the contents of a section header and store it. 
 * 
 *  shdr_pointer: section header
 *	execfile: executable file stream to be read
 */
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
 * Checks the magic number to verify if it's an ELF file
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

/*
 * Function:  read_program_entries 
 * --------------------
 * Read all entries of the program headers and store them. 
 * 
 *  phdr: program header
 *  _phnum: program headers number
 *  ph_offset: program header offset
 *  entry_size: size of each entry
 *	execfile: executable file stream to be read
 */
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

		fread(ehdr_pointer->e_ident, sizeof(char), EI_NIDENT, *execfile);
		
		if (check_e_Ident(ehdr_pointer->e_ident) != -1)
		{	
			//Read each term in the ELF Header
			read_elf_header(ehdr_pointer, execfile_pointer);
			num_program_entries = (uint16_t) ehdr_pointer->e_phnum;
			program_table_header = (Elf32_Phdr *) malloc(num_program_entries * sizeof(Elf32_Phdr));
			read_program_entries(program_table_header, num_program_entries, ehdr_pointer->e_phoff, 
				ehdr_pointer->e_phentsize, execfile_pointer);

			return program_table_header;
		}
		else 
		{
			fprintf(stderr, "File isn't in proper ELF format: \"%s\" \n", filename);
			return NULL;
		}
	}
	else
	{	
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

/*
 * Function:  read_sections
 * --------------------
 * Loop through all sections; Read each Header and section content
 * 
 *  execfile: executable file stream
 *	sections_buffer: the buffer with the read content
 *	offset: offset to the entry location in the file
 *  sections_headers
 *  elf_header         
 */
void read_sections(FILE *execfile, unsigned char **sections_buffer, Elf32_Shdr* sections_headers, Elf32_Ehdr *elf_header)
{	
	uint16_t section_header_size = elf_header->e_shentsize;
	uint16_t num_sections = elf_header->e_shnum;
	uint32_t sections_offset =  elf_header->e_shoff;

	for (int i = 0; i < num_sections; i++)
	{	
		//sections_headers[i] = (Elf32_Shdr*) malloc(sizeof(Elf32_Shdr));
		// Offsets the file cursor from the beginning to the Section Header table
		fseek(execfile, sections_offset + i*section_header_size, SEEK_SET);		
		read_section_header(&(sections_headers[i]), execfile);

		read_entry(execfile, &(sections_buffer[i]), sections_headers[i].sh_offset, sections_headers[i].sh_size);
	}
}

/*
 * Function:  write_sections
 * --------------------
 * Loop through all sections; Read each Header and section content
 * 
 *  imagefile
 *	sections_buffer: the buffer with the read content
 *  sections_headers
 *  num_sections
 *	image_offset: offset to the entry location in the image file
 */
void write_sections(FILE **imagefile, unsigned char **sections_buffer, Elf32_Shdr* sections_headers, 
	uint32_t num_sections, uint32_t image_offset)
{
	for (int i = 0; i < num_sections; i++)
	{	
		if (sections_headers[i].sh_addr != 0)  /* This member gives the address at which the section’s first byte       */ 
		{									   /* should reside. If this member == 0, the section should not be written.*/	
			// Offsets imagefile cursor from the beginning to the given section address
			printf("A CARALHO SECTION ESCRITA i = %d\n", i);
			fseek(*imagefile, sections_headers[i].sh_addr + image_offset, SEEK_SET);
			fwrite(sections_buffer[i], 1, sections_headers[i].sh_size, *imagefile);
		}
		free(sections_buffer[i]);
	}
}

/*
 * Function:  write_program_segments
 * --------------------
 * Loop through all sections; Read each Header and section content
 * 
 *  imagefile
 *	program_buffer: the buffer with the read content
 *  program_header
 *  num_programs
 *	image_offset: offset to the entry location in the image file
 */
void write_program_segments(FILE **imagefile, unsigned char **program_buffer, Elf32_Phdr *program_header, 
	uint32_t num_programs, uint32_t image_offset)
{	
	uint32_t padding_size; 		  /* When the segment size in memory is bigger than  */
	unsigned char *padded_buffer; /* it's size in file, it must be zero-padded.     */

	for (int i = 0; i < num_programs; i++) 		
	{
		fseek(*imagefile, program_header[i].p_vaddr + image_offset, SEEK_SET);
		fwrite(program_buffer[i], 1, program_header[i].p_filesz, *imagefile);
		
		// Verify if padding is needed and do it
		padding_size = program_header[i].p_memsz - program_header[i].p_filesz;
		if(padding_size > 0)
		{
			padded_buffer = (unsigned char *) calloc(padding_size, sizeof(unsigned char));
			fwrite(padded_buffer, 1, padding_size, *imagefile);
			free(padded_buffer);
		}
		free(program_buffer[i]);
	}
}

/*
 * Function:  read_program_segments
 * --------------------
 * Loop through all programs; Read each Header and segment content
 * 
 *  execfile: executable file stream
 *	program_buffer: the buffer with the read content
 *  program_header
 *  num_programs
 */
void read_program_segments(FILE *execfile, unsigned char **program_buffer, Elf32_Phdr *program_header, uint16_t num_programs)
{
	for (int i = 0; i < num_programs; i++)
	{	
		read_entry(execfile, &(program_buffer[i]), program_header[i].p_offset, program_header[i].p_filesz);
	}
}

/*
 * Function:  write_bootblock
 * --------------------
 * Writes the bootblock to the image file
 * 
 *  imagefile
 * 	bootfile
 *  boot_header: bootblock elf header
 *  boot_phdr: bootblock program header
 */
void write_bootblock(FILE **imagefile, FILE *bootfile, Elf32_Ehdr *boot_header, Elf32_Phdr *boot_phdr)
{	
	uint16_t num_sections = boot_header->e_shnum;
	uint16_t num_programs = boot_header->e_phnum;

	// Allocate sections for reading
	Elf32_Shdr* sections_headers = (Elf32_Shdr*) malloc(num_sections * sizeof(Elf32_Shdr));

	// Buffer to store the content of each section
	unsigned char **sections_buffer = (unsigned char **) malloc(num_sections * sizeof(unsigned char*));
	// Buffer to store the content of each program segment
	unsigned char **program_buffer = (unsigned char **) malloc(num_programs * sizeof(unsigned char*));

	read_program_segments(bootfile, program_buffer, boot_phdr, num_programs);
	write_program_segments(imagefile, program_buffer, boot_phdr, num_programs, BOOTBLOCK_IMAGE_OFFSET);	

	read_sections(bootfile, sections_buffer, sections_headers, boot_header);	
	write_sections(imagefile, sections_buffer, sections_headers, boot_header->e_shnum, BOOTBLOCK_IMAGE_OFFSET);

	free(sections_headers);
	free(sections_buffer);
	free(program_buffer);
}

/*
 * Function:  write_kernel
 * --------------------
 * Writes the kernel to the image file
 * 
 *  imagefile
 * 	kernelfile
 *  kernel_header: kernel elf header
 *  kernel_phdr: kernel program header
 */
void write_kernel(FILE **imagefile, FILE *kernelfile, Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{
	uint16_t num_sections = kernel_header->e_shnum;
	uint16_t num_programs = kernel_header->e_phnum;
	
	// Allocate sections for reading
	Elf32_Shdr* sections_headers = (Elf32_Shdr*) malloc(num_sections * sizeof(Elf32_Shdr));

	// Buffer to store the content of each section
	unsigned char **sections_buffer = (unsigned char **) malloc(num_sections * sizeof(unsigned char*));
	// Buffer to store the content of each program segment
	unsigned char **program_buffer = (unsigned char **) malloc(num_programs * sizeof(unsigned char*));

	read_program_segments(kernelfile, program_buffer, kernel_phdr, num_programs);
	write_program_segments(imagefile, program_buffer, kernel_phdr, num_programs, KERNEL_IMAGE_OFFSET);	

	read_sections(kernelfile, sections_buffer, sections_headers, kernel_header);	
	write_sections(imagefile, sections_buffer, sections_headers, kernel_header->e_shnum, KERNEL_IMAGE_OFFSET);

	free(sections_headers);
	free(sections_buffer);
	free(program_buffer);
}

/*
 * Function:  count_kernel_sectors
 * --------------------
 * Counts the number of sectors in the kernel 
 * 
 *  kernel_header: kernel elf header
 *  kernel_phdr: kernel program header
 * 
 *  returns: number of sectors in the kernel
 */
int count_kernel_sectors(Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{	
	uint32_t sum_memsz = 0;
	uint32_t num_sectors;

	for (int i = 0; i < kernel_header->e_phnum; i++)
	{
		sum_memsz += kernel_phdr[i].p_memsz;
	}

	num_sectors = sum_memsz / SECTOR_SIZE;

	if (sum_memsz % SECTOR_SIZE > 0) 
		num_sectors++;

	return num_sectors;
}

/*
 * Function:  record_kernel_sectors
 * --------------------
 * Records the number of sectors in the kernel
 * 	
 * 	imagefile
 *  kernel_header: kernel elf header
 *  kernel_phdr: kernel program header
 * 	num_sec: number of kernel sectors
 */
void record_kernel_sectors(FILE **imagefile, Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr, int num_sec)
{
	unsigned char magic_number[2] = {0xAA, 0x55};
	fseek(*imagefile, BOOTLOADER_KERNEL_SECTORS_OFFSET, SEEK_SET);
	fwrite(&num_sec, 1, 1, *imagefile);
	// Write magic Number
	fseek(*imagefile, BOOTLOADER_SIG_OFFSET, SEEK_SET);
	fwrite(magic_number, 2, 1, *imagefile);
}

/*
 * Function:  print_segments_info
 * --------------------
 * Prints the offset, vaddr, filesz, memsz and other info of 
 * each segment of a given program header
 * 	
 * 	program_header
 *  _phnum: number of program headers
 *  is_kernel: TRUE if the program_header is a kernel - used for padding calculation
 */
void print_segments_info(Elf32_Phdr *program_header, int _phnum, int is_kernel) 
{
	int num_sectors = 0;
	for(int i = 0; i < _phnum; i++)
	{
		num_sectors += (program_header[i].p_memsz/512 + ((program_header[i].p_memsz % SECTOR_SIZE) > 0 ? 1 : 0) + is_kernel);
		printf("\tsegment %d\n", i);
		printf("\t\toffset 0x%04x\t\tvaddr 0x%04x\n", program_header[i].p_offset, program_header[i].p_vaddr);
		printf("\t\tfilesz 0x%04x\t\tmemsz 0x%04x\n", program_header[i].p_filesz, program_header[i].p_memsz);
		printf("\t\twriting 0x%04x bytes\n", program_header[i].p_memsz);
		printf("\t\tpadding up to 0x%04x\n", SECTOR_SIZE * num_sectors);
	}
}

/*
 * Function:  extended_opt
 * --------------------
 * Prints segment information for --extended option
 * 	
 * 	bph: bootfile program header
 *  k_phnum: kernel number of program headers
 *  kph: kernelfile program header
 *  num_sec: number of kernel sectors
 */
void extended_opt(Elf32_Phdr *bph, int k_phnum, Elf32_Phdr *kph, int num_sec)
{
	/* print number of disk sectors used by the image */
	printf("disk_sectors: %d\n", num_sec + 1);

	/*bootblock segment info */
	printf("0x%04x: ./bootblock\n", bph->p_vaddr);
	print_segments_info(bph, 1, FALSE);
	
	/* print kernel segment info */
	printf("0x%04x: ./kernel\n", kph->p_vaddr);
	print_segments_info(kph, k_phnum, TRUE);

	/* print kernel size in sectors */
	printf("os_size: %d sectors\n", num_sec);
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
	
	int num_sectors; // number of kernel sectors

	//TODO: change this for the second project
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
	
	/* write bootblock */
	write_bootblock(&imagefile, bootfile, boot_header, boot_program_header);

	/* read executable kernel file */
	kernel_program_header = read_exec_file(&kernelfile, argv[kernel_arg(argc)], &kernel_header);

	/* write kernel segments to image */
	write_kernel(&imagefile, kernelfile, kernel_header, kernel_program_header);

	num_sectors = count_kernel_sectors(kernel_header, kernel_program_header);
	/* tell the bootloader how many sectors to read to load the kernel */
	record_kernel_sectors(&imagefile, kernel_header, kernel_program_header, num_sectors);

	/* check for  --extended option */
	if (!strncmp(argv[1], "--extended", 11))
	{
		/* print info */
		extended_opt(boot_program_header, kernel_header->e_phnum, kernel_program_header, num_sectors);
	} 

	fclose(imagefile);
	fclose(bootfile);
	fclose(kernelfile);

	free(boot_header);
	free(kernel_header);
	free(kernel_program_header);
	free(boot_program_header);
	
	return 0;
} // ends main()


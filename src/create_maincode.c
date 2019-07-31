
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#define OS_LINUX 0
#define OS_WIN 1
#define OS_TYPE OS_LINUX


#if 1
#define DPRINTF() (printf("[%05d]-%s(). Here---------\n", __LINE__, __FUNCTION__))
#else
#define DPRINTF() do{}while(0)
#endif


#define CONST_IFILE_LEN 128
#define CONST_OFILE_LEN (CONST_IFILE_LEN + 16)
typedef struct{
	char ifile[CONST_IFILE_LEN];
	char ofile[CONST_OFILE_LEN];
}INPUT_INFO_T;

#define FTYPE_DIR 4
#define FTYPE_FILE 8

INPUT_INFO_T s_input = {0};


static void print_exe_help(void)
{
	printf("--------------------\n");
	printf("./create_maincode  -ifile ./download_ecos.bin\n");
	printf("\n");
}
static int parse_input_value(INPUT_INFO_T info, int argc, char *argv[])
{
	int result =-1;

	if(argc<=1)
	{
		print_exe_help();
		return result;
	}
	printf("argc = %d\n", argc);

	int i=0;
	for(i=0; i<argc; i++)
	{
		printf("argv[%d] = %s\n", i, argv[i]);
		if(argv[i][0] == '-')
		{
			if(strcasecmp(argv[i], "-ifile")==0)
			{
				if(++i < argc)
				{
					printf("argv[%d] = %s\n", i, argv[i]);
					result =1;
					strncpy(s_input.ifile, argv[i], CONST_IFILE_LEN);
					snprintf(s_input.ofile, CONST_OFILE_LEN, "maincode_%s",  s_input.ifile);
				}
				else
				{
					printf("input <%d> format ERR!!!\n", i);
					result =-1;
					break;
				}
			}
			else if(strcasecmp(argv[i], "-help")==0)
			{
				result =0;
				print_exe_help();
				break;
			}
		}
	}

	if(result<0)
	{
		print_exe_help();
	}
	return result;
}

static int create_maincode(INPUT_INFO_T *pinfo)
{
	FILE *pifile = NULL;
	FILE *pofile = NULL;
	pifile = fopen(pinfo->ifile, "rb");
	if(pifile == NULL)
	{
		printf("Err<2>, file:%s not exit.\n", pinfo->ifile);
		return 2;
	}



	pofile = fopen(pinfo->ofile, "wb+");
	if(pofile == NULL)
	{
		printf("Err<2>, file:%s open failed.\n", pinfo->ofile);
		fclose(pifile);
		return 2;
	}

	long skip_len = 65536*3;
	int result = fseek(pifile, skip_len, SEEK_SET);
	if(result<0)
	{
		printf("Err<3>, file len < 65556*3K.\n");
		fclose(pifile);
		fclose(pofile);
		return 3;
	}

	void *buf[65536];
	int count =0;
	while(fread(buf, 65536, 1, pifile))
	{
		++count;
		fwrite(buf, 65536, 1, pofile);
		if(count == 53)
		{
			printf("create file: %s success!\n",  pinfo->ofile);
			break;
		}
	}
	fclose(pifile);
	fclose(pofile);

	return 0;
}

typedef struct file_nope{
	char *filename;
	struct file_nope *next;
}FILELIST_NOPE_T;

static FILELIST_NOPE_T s_filelist;

static void _add_file_to_filelist(const char *filename)
{
	static FILELIST_NOPE_T *fnope = &s_filelist;
	static FILELIST_NOPE_T *father = &s_filelist;
	if(filename == NULL)
	{
		return ;
	}

	if(fnope == NULL)
	{
		fnope =(FILELIST_NOPE_T*)malloc(sizeof(FILELIST_NOPE_T*));
		memset(fnope, 0, sizeof(FILELIST_NOPE_T));
		father->next = fnope;
	}
	fnope->filename = strdup(filename);
	printf("file name =%s\n", fnope->filename);
	father = fnope;
	fnope = fnope->next;

}

#if OS_TYPE == OS_LINUX
static void get_filelist(FILELIST_NOPE_T *filelist)
{
	char dirName[128];
	getcwd(dirName, sizeof(dirName));
	printf("current dir =%s\n", dirName);

	DIR *pDir = NULL;
	pDir = opendir(dirName);
	if(pDir == NULL)
	{
		printf("Open dir:%s failed!\n", dirName);
	}

	struct dirent *ptr = NULL;
	while(ptr=readdir(pDir))
	{
		if(ptr->d_type == FTYPE_FILE && (strncmp(ptr->d_name, "maincode_",9)!=0))
		{
			if(strstr(ptr->d_name, ".bin")==NULL)
			{
				continue;
			}
			_add_file_to_filelist(ptr->d_name);
		}
	}
}
#elif OS_TYPE == OS_WIN
#include <io.h>
static void get_filelist(FILELIST_NOPE_T *filelist)
{
	char dirName[128];
	getcwd(dirName, sizeof(dirName));
	printf("current dir =%s\n", dirName);


	struct _finddata_t fa;
	long handle;

    if((handle = _findfirst(strcat(dirName, "\\*.bin"), &fa)) == -1L)
    {
        printf("The Path %s is wrong!\n",dirName);
        return ;
	}

	do
	{
		printf("%s\n",fa.name);
		if(strncmp(fa.name, "maincode_",9)!=0)
			_add_file_to_filelist(fa.name);
	}while(_findnext(handle, &fa) ==0);

	_findclose(handle);
}
#else /*also windows*/
#include "windows.h"
static void get_filelist(FILELIST_NOPE_T *filelist)
{
	char dirName[128];
	getcwd(dirName, sizeof(dirName));
	printf("current dir =%s\n", dirName);

	WIN32_FIND_DATA fdFileData;
	HANDLE hFindFile;

	if((hFindFile = FindFirstFile(strcat(dirName,"\\*.bin"), &fdFileData)) == INVALID_HANDLE_VALUE)
	{
		printf("Find file failed, Error code:%d\n", GetLastError());
		return ;
	}
	do {
		if(strncmp(fdFileData.cFileName, "maincode_",9)!=0)
 			_add_file_to_filelist(fdFileData.cFileName);
	}while(FindNextFile(hFindFile, &fdFileData));
	FindClose(hFindFile);
}
#endif
static void free_filelist(FILELIST_NOPE_T *filelist)
{
	FILELIST_NOPE_T *fnode = filelist;
	FILELIST_NOPE_T *ftemp = NULL;
	if(fnode && fnode->filename)
	{
		free(fnode->filename);
		fnode->filename = NULL;
		fnode = fnode->next;
	}
	while(fnode)
	{
		if(fnode->filename)
			free(fnode->filename);
		ftemp = fnode;
		fnode = fnode->next;
		free(ftemp);
	}

}

static void reset_input_data(INPUT_INFO_T *pinfo, const char *filename)
{
	memset(pinfo, 0, sizeof(INPUT_INFO_T));
	strncpy(pinfo->ifile, filename, CONST_IFILE_LEN);
	snprintf(pinfo->ofile, CONST_OFILE_LEN, "maincode_%s", filename);
}

int  main(int argc , char *argv[])
{
	int result =0;
#if 0 /*get input-info from user*/
	result = parse_input_value(s_input, argc, argv);
	if(result <=0)
	{	printf("Err<1>, input info err.\n");
		return 1;
	}
#else	/*get input-info by auto<current path>*/
	get_filelist(&s_filelist);

	FILELIST_NOPE_T *fnope = &s_filelist;

	while(fnope && fnope->filename)
	{
		reset_input_data(&s_input, fnope->filename);
		result = create_maincode(&s_input);
		fnope = fnope->next;
	}

	free_filelist(&s_filelist);
#endif
    return result;
}


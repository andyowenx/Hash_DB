#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#define INDEX_FILENAME "index"
#define DATA_FILENAME "data"
#define MAX_HASH 2000
typedef struct{
		char *name;
		char *db_filename;
		bool live;
		long size;
		long offset;
}NODE;
FILE*create_index();
int read_index(FILE*index_fd,FILE*data_fd,char*key_list[2000]); //return the key count
NODE*init_node();
NODE*full_node(char input[1000],FILE*data_fd,NODE*n);
void printf_data(FILE*data_fd,NODE*n);
void help_message();
void insert_data(FILE*data_fd,char*filename,char*key_list[2000],int*count);
void show_data(FILE*data_fd,char*filename,char*key_list[2000],int*count);
void delete_data(char*filename);
void write_index(FILE*index_fd,char*key_list[2000],int*count);
int main()
{
		char input[100];
		char *ptr;
		char *key_list[2000];
		int count;
		FILE *index_fd,*data_fd;
		hcreate(MAX_HASH);
		if (!(data_fd=fopen(DATA_FILENAME,"a+"))){
				perror("Cannot read data file.\n");
				exit(1);
		}
		if (!(index_fd=fopen(INDEX_FILENAME,"a+"))){
				//There is no record in Odb , create one
				index_fd=create_index();
				count=0;
		}
		else{
				count=read_index(index_fd,data_fd,key_list);
		}
		//user command line
		help_message();
		while (1){
				printf("\n>> ");
				fflush(stdout);
				fgets(input,100,stdin);
				if (input[strlen(input)-1]=='\n')
						input[strlen(input)-1]='\0';
				ptr=strtok(input," ");
				if (ptr==NULL)
						continue;
				else if (strcasecmp(ptr,"insert")==0||strcasecmp(ptr,"i")==0){
						ptr=strtok(NULL," ");
						if (!ptr){
								printf("Please input your filename .\nIf it is at another folder ,please input the whole path.\n");
								continue;
						}
						else
								insert_data(data_fd,ptr,key_list,&count);
				}
				else if (strcasecmp(ptr,"show")==0||strcasecmp(ptr,"s")==0){
						ptr=strtok(NULL," ");
						if (!ptr)
								show_data(data_fd,"*",key_list,&count);
						else
								show_data(data_fd,ptr,key_list,&count);
				}
				else if (strcasecmp(ptr,"delete")==0||strcasecmp(ptr,"d")==0){
						ptr=strtok(NULL," ");
						if (!ptr){
								printf("Please input the filename you want to delete.\n");
								continue;
						}
						else
								delete_data(ptr);
				}
				else if(strcasecmp(ptr,"help")==0||strcasecmp(ptr,"h")==0)
						help_message();
				else if (strcasecmp(ptr,"quit")==0||strcasecmp(ptr,"q")==0)
						break;
				else{
						printf("Wrong input.\n");
						help_message();
				}
		}
		//rewrite the index file
		write_index(index_fd,key_list,&count);
		fclose(data_fd);
		return 0;
}
void write_index(FILE*index_fd,char*key_list[2000],int*count)
{
		fseek(index_fd,0,SEEK_END);
		long index_size=ftell(index_fd);
		int i;
		ENTRY e,*ep;
		NODE*n;
		fseek(index_fd,0,SEEK_SET);
		for (i=0;i<*count;i++){
				e.key=key_list[i];
				ep=hsearch(e,FIND);
				if (ep){
						n=(NODE*)ep->data;
						if (n->live==true)
								fprintf(index_fd,"%s,%s,%ld,%ld\n",n->name,n->db_filename,n->size,n->offset);
				}
		}
		while (ftell(index_fd)<index_size)
				fputc('\0',index_fd);
		fclose(index_fd);
}
NODE*init_node()
{
		NODE*n=(NODE*)malloc(sizeof(NODE));
		if (!n){
				perror("Cannot malloc node.\n");
				exit(1);
		}
		n->name=NULL;
		n->db_filename=NULL;
		n->live=false;
		n->size=0;
		n->offset=0;
		return n;
}
NODE*full_node(char input[1000],FILE*data_fd,NODE*n)
{
		char *p;
		p=strtok(input,",");
		n->name=(char*)malloc(strlen(p)*sizeof(char));
		strcpy(n->name,p);
		p=strtok(NULL,",");
		//input from index_fd
		if (p!=NULL){
				n->live=true;
				n->db_filename=(char*)malloc(strlen(p)*sizeof(char));
				strcpy(n->db_filename,p);
				p=strtok(NULL,",");
				n->size=atoll(p);
				p=strtok(NULL,",");
				if (p[strlen(p)-1]=='\n')
						p[strlen(p)-1]='\0';
				n->offset=atoll(p);
		}
		//input from user
		else{
				n->db_filename=(char*)malloc(strlen(DATA_FILENAME)*sizeof(char));
				strcpy(n->db_filename,DATA_FILENAME);
				n->live=true;
				FILE*fd;
				char buf[4096];
				if (!(fd=fopen(n->name,"r"))){
						perror("Read target file error.\n");
						exit(1);
				}
				fseek(fd,0,SEEK_END);
				n->size=ftell(fd);
				fseek(data_fd,1,SEEK_END);
				n->offset=ftell(data_fd);
				//write in data_fd
				fseek(fd,0,SEEK_SET);
				while ((fread(buf,sizeof(buf),1,fd))>0){
						fwrite(buf,strlen(buf),1,data_fd);
				}
				fclose(fd);
		}
		return n;
}
int read_index(FILE*index_fd,FILE*data_fd,char *key_list[2000])
{
		char input[1000];
		char*ptr;
		int count=0;
		while (fgets(input,1000,index_fd)!=NULL){
				insert_data(data_fd,input,key_list,&count);
		}
		return count;
}
void insert_data(FILE*data_fd,char*filename,char *key_list[2000],int*count)
{
		ENTRY*ep,e;
		NODE*n=init_node();
		n=full_node(filename,data_fd,n);
		//record the key
		key_list[*count]=(char*)malloc(strlen(n->name)*sizeof(char));
		strcpy(key_list[*count],n->name);
		(*count)++;
		//insert into hash table
		e.key=n->name;
		e.data=n;
		ep=hsearch(e,ENTER);
		if (!ep){
				perror("Enter ENTRY error.\n");
				exit(1);
		}
}

void show_data(FILE*data_fd,char*filename,char*key_list[2000],int*count)
{
		ENTRY e,*ep;
		NODE*n;
		if (strcasecmp(filename,"*")==0){
				int i;
				for (i=0;i<*count;i++){
						e.key=key_list[i];
						ep=hsearch(e,FIND);
						n=(NODE*)ep->data;
						if (n->live==true)
								printf_data(data_fd,n);
				}
		}
		else{
				e.key=filename;
				ep=hsearch(e,FIND);
				if (ep){
						n=(NODE*)ep->data;
						printf_data(data_fd,n);
				}
				else
						printf("Cannot find this filename.\n");
		}
}
void printf_data(FILE*data_fd,NODE*n)
{
		char buf[4096];
		long remain_size;
		printf("%s,%s,%ld,%ld\n",n->name,n->db_filename,n->size,n->offset);
		fseek(data_fd,n->offset,SEEK_SET);
		remain_size=n->size;
		while (remain_size>0){
				if (remain_size>=4096){
						remain_size-=4096;
						fread(buf,sizeof(buf),1,data_fd);
						write(stdout,buf,sizeof(buf));
				}
				else{
						fread(buf,remain_size*sizeof(char),1,data_fd);
						write(stdout,buf,remain_size*sizeof(char));
						remain_size=0;
				}
		}
}
void delete_data(char*filename)
{
		ENTRY e,*ep;
		e.key=filename;
		ep=hsearch(e,FIND);
		NODE*n=(NODE*)ep->data;
		if (ep){
				n->live=false;
				printf("Delete success.\n");
		}
		else
				printf("Delete fail , cannot find this file.\n");
}

void help_message()
{
		printf("Insert/i\t[filename]\n");
		printf("Show/s\t[filename]\n");
		printf("Delete/d\t[filename]\n");
		printf("Help/h\n");
		printf("Quit/q\n");
}
FILE*create_index()
{
		FILE*fd;
		if(!(fd=fopen(INDEX_FILENAME,"w+"))){
				perror("Cannot create index file.\n");
				exit(1);
		}
		return fd;
}
